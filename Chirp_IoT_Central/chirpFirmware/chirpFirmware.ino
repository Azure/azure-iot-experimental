#include "Arduino.h"
#include "AZ3166WiFi.h"

// IoT device SDK
#include "AzureIotHub.h"

// Sensors
#include "HTS221Sensor.h"
#include "LPS22HBSensor.h"
#include "LIS2MDLSensor.h"
#include "RGB_LED.h"

// EEPROM
#include "EEPROMInterface.h"

// JSON library
#include "parson.h"

// Chirp and Audio handler
#include "chirp_connect.h"
#include "AudioClassV2.h"

#define IOT_CENTRAL_ZONE_IDX      0x02
#define IOT_CENTRAL_MAX_LEN       128

// Helper macros
#define randVal(min, max) ((int)random(min, max))
#define RSV() (randVal(-2000, 2000))  // because I'm lazy RSV = Random Sensor Value

// debounce time for the A and B buttons
#define switchDebounceTime 250

// is the device configured
bool configured = false;
char key[6] = "";

// global Azure IoT Hub client handle
IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

// set to true if you want detailed logging from the Azure IoT device SDK
static const bool traceOn = true;

// set to true if you want to see payload details in the logging OUTPUT
static const bool payloadLogging = true;

// callback context definition
typedef struct EVENT_INSTANCE_TAG {
    IOTHUB_MESSAGE_HANDLE messageHandle;
    int messageTrackingId; // For tracking the messages within the user callback.
} EVENT_INSTANCE;

// counters for display and context
static int trackingId = 0;
static int errorCount = 0;
static int sentCount = 0;
static int ackCount = 0;

// are we connected to wifi
static bool connected = false;

// last action timers for the main loop
unsigned long lastTelemetrySend = 0;
unsigned long lastSwitchPress = 0;

// sensor variables
static DevI2C *i2c;
static LPS22HBSensor *pressure;
static HTS221Sensor *tempHumidity;
static RGB_LED rgbLed;

//
// simple base64 encoder to prevent having to use an Arduino library
// See https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64
//
void base64encode(const void* data_buf, size_t dataLength, char* result, size_t resultSize)
{
    const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const uint8_t *data = (const uint8_t *)data_buf;
    size_t resultIndex = 0;
    size_t x;
    uint32_t n = 0;
    int padCount = dataLength % 3;
    uint8_t n0, n1, n2, n3;

    for (x = 0; x < dataLength; x += 3) 
    {
        n = ((uint32_t)data[x]) << 16;
        if((x+1) < dataLength)
            n += ((uint32_t)data[x+1]) << 8;
        if((x+2) < dataLength)
            n += data[x+2];

        /* this 24-bit number gets separated into four 6-bit numbers */
        n0 = (uint8_t)(n >> 18) & 63;
        n1 = (uint8_t)(n >> 12) & 63;
        n2 = (uint8_t)(n >> 6) & 63;
        n3 = (uint8_t)n & 63;

        result[resultIndex++] = base64chars[n0];
        result[resultIndex++] = base64chars[n1];

        if((x+1) < dataLength)
            result[resultIndex++] = base64chars[n2];

        if((x+2) < dataLength)
            result[resultIndex++] = base64chars[n3];
    }
    if (padCount > 0) { 
        for (; padCount < 3; padCount++) { 
            result[resultIndex++] = '=';
        } 
    }
    result[resultIndex] = 0;
}

//
// EEPROM store and retrieve
//
void storeWiFi(const char *ssid, const char *password) {
    EEPROMInterface eeprom;

    eeprom.write((uint8_t*)ssid, strlen(ssid), WIFI_SSID_ZONE_IDX);
    eeprom.write((uint8_t*)password, strlen(password), WIFI_PWD_ZONE_IDX);
}

void storeConnectionString(const char *connectionString) {
    EEPROMInterface eeprom;
    eeprom.write((uint8_t*)connectionString, strlen(connectionString), AZ_IOT_HUB_ZONE_IDX);
}

void readWiFi(char* ssid, int ssidLen, char *password, int passwordLen) {
    EEPROMInterface eeprom;
    eeprom.read((uint8_t*)ssid, ssidLen, 0, WIFI_SSID_ZONE_IDX);
    eeprom.read((uint8_t*)password, passwordLen, 0, WIFI_PWD_ZONE_IDX);
}

String readConnectionString() {
    EEPROMInterface eeprom;
    uint8_t connectionString[AZ_IOT_HUB_MAX_LEN];
    eeprom.read((uint8_t*)connectionString, AZ_IOT_HUB_MAX_LEN, 0, AZ_IOT_HUB_ZONE_IDX);
    return String((const char*)connectionString);
}

void clearWiFi() {
    EEPROMInterface eeprom;
    
    uint8_t *cleanBuff = (uint8_t*) malloc(WIFI_SSID_MAX_LEN);
    memset(cleanBuff, 0x00, WIFI_SSID_MAX_LEN);
    eeprom.write(cleanBuff, WIFI_SSID_MAX_LEN, WIFI_SSID_ZONE_IDX);
    free(cleanBuff);

    cleanBuff = (uint8_t*) malloc(WIFI_PWD_MAX_LEN);
    memset(cleanBuff, 0x00, WIFI_PWD_MAX_LEN);
    eeprom.write(cleanBuff, WIFI_PWD_MAX_LEN, WIFI_PWD_ZONE_IDX);
    free(cleanBuff);
}

void clearConnectionString() {
    EEPROMInterface eeprom;

    uint8_t *cleanBuff = (uint8_t*) malloc(AZ_IOT_HUB_MAX_LEN);
    memset(cleanBuff, 0x00, AZ_IOT_HUB_MAX_LEN);
    eeprom.write(cleanBuff, AZ_IOT_HUB_MAX_LEN, AZ_IOT_HUB_ZONE_IDX);
    free(cleanBuff);
}

// this function is needed to yield to the hub SDK to process messages to and from the hub
void hubClientYield(void) {
    int waitTime = 1;
    IoTHubClient_LL_DoWork(iotHubClientHandle);
    ThreadAPI_Sleep(waitTime);
}

// callback to process confirmations from reported property device twin operations
static void deviceTwinConfirmationCallback(int status_code, void* userContextCallback) {
    Serial.printf("DeviceTwin CallBack: Status_code = %u\r\n", status_code);
}

// send reported properties to the IoT hub
bool sendReportedProperty(const char *payload) {
    bool retValue = true;
    
    IOTHUB_CLIENT_RESULT result = IoTHubClient_LL_SendReportedState(iotHubClientHandle, (const unsigned char*)payload, strlen(payload), deviceTwinConfirmationCallback, NULL);

    if (result != IOTHUB_CLIENT_OK) {
        Serial.println("Failure sending reported property!!!");
        retValue = false;
    }

    return retValue;
}

// callback to process device twin desired property changes
static void deviceTwinGetStateCallback(DEVICE_TWIN_UPDATE_STATE update_state, const unsigned char* payLoad, size_t size, void* userContextCallback) {
    if (payloadLogging) {
        Serial.println((char*)payLoad);
    }

    JSON_Value *root_value;
    root_value = json_parse_string((const char*)payLoad);
    float voltage = -1;
    int desiredVersion;

    if (DEVICE_TWIN_UPDATE_COMPLETE == update_state) {  // process a full desired properties payload sent during initial connection

        voltage = json_object_dotget_number(json_object(root_value), "desired.setVoltage.value");
        desiredVersion = json_object_dotget_number(json_object(root_value), "desired.$version");

    } else {  // process a partial/patch desired property payload
        voltage = json_object_dotget_number(json_object(root_value), "setVoltage.value");
        desiredVersion = json_object_get_number(json_object(root_value), "$version");
    }

    if (voltage > -1) {
        Serial.printf("voltage: %f\r\n", voltage);

        if (voltage <= 100) {
            //set RGB LED to green
            rgbLed.setColor(0, 255, 0);
        } else if (voltage > 100 && voltage <=200) {
            // set RGB LED to blue
            rgbLed.setColor(0, 0, 255);
        } else if (voltage > 200) {
            // set RGB LED to red
            rgbLed.setColor(255, 0, 0);
        }
    }

    // acknowledge the desired state
    char buff[1024];
    sprintf(buff, "{\"setVoltage\":{\"value\":%f, \"statusCode\":%d, \"status\":\"%s\", \"desiredVersion\":%d}}", voltage, 200, "completed", desiredVersion);
    if (payloadLogging) {
        Serial.println(buff);
    }
    sendReportedProperty(buff);
}

#define SAMPLE_RATE 16000
#define AUDIO_SAMPLE_SIZE 16

// these need to be defined by the user from https://developer.chirp.io
#define APP_KEY "<replace with application key from Chirp>"
#define APP_SECRET "<replace with application secret from Chirp>"
#define APP_LICENCE "<replace with licence key from Chirp>"

char chirp_ssid[64] = {0};
char chirp_pwd[64] = {0};
char chirp_connection_string[256] = {0};

bool chirp_heard = false;
bool chirp_decode_ok = false;

typedef enum {
  BUFFER_STATE_NONE,
  BUFFER_STATE_EMPTY,
  BUFFER_STATE_FULL,
} bufferState;

static AudioClass& Audio = AudioClass::getInstance();
static int16_t shortDecoderBuffer[AUDIO_CHUNK_SIZE / 2] = {0};
static float floatDecoderBuffer[AUDIO_CHUNK_SIZE / 4] = {0};

bufferState decoderBufferState = BUFFER_STATE_EMPTY;

chirp_connect_t *chirpConnect = NULL;

void to_hex(char *hexStr, const char *binStr)
{
    memset(hexStr,0,sizeof(hexStr));
    int i;
    int j;
    for(i=0,j=0;i<strlen(binStr);i++,j+=2)
    { 
        sprintf((char*)hexStr+j,"%02x",binStr[i]);
    }
    hexStr[j]='\0';
}

const char *chirp_get_host_name(char *packed_host_name)
{
    char *hostname;
    hostname = (char*)malloc(100);
    memset(hostname, 0, 100);
    char hexStr[33];
    to_hex(hexStr, packed_host_name);
    sprintf(hostname, "HostName=saas-iothub-%.8s-%.4s-%.4s-%.4s-%.12s.azure-devices.net", hexStr, hexStr+8, hexStr+12, hexStr+16, hexStr+20);
    Serial.printf("%s\n", hostname);
    return hostname;
}

const char *chirp_get_device_id(char *packed_device_id)
{
    char *deviceId;
    deviceId = (char*)malloc(strlen((const char*)packed_device_id)+10);
    memcpy(deviceId, 0, strlen((const char*)packed_device_id)+10);
    sprintf(deviceId, "DeviceId=%s", packed_device_id);
    Serial.printf("%s\n", deviceId);
    return deviceId;
}

const char *chirp_get_access_key(char *packed_access_key)
{
    char accessKey[100];
    base64encode(packed_access_key, 32, accessKey, 100);
    char *key;
    key = (char*)malloc(strlen((const char*)accessKey)+17);
    memcpy(key, 0, strlen((const char*)accessKey)+17);
    sprintf(key, "SharedAccessKey=%s", accessKey);
    Serial.printf("%s\n", key);
    return key;
}

char *chirp_reconstruct_connection_string(const char *packed_connection_string)
{
    size_t packed_connection_string_length = strlen(packed_connection_string);

    auto *packed_connection_string_copy = (char *) calloc(packed_connection_string_length + 1, sizeof(char));
    strncpy(packed_connection_string_copy, packed_connection_string, packed_connection_string_length);

    char *save_ptr = packed_connection_string_copy;
    char *packed_host_name = strsep(&packed_connection_string_copy, ";");
    char *packed_device_id = strsep(&packed_connection_string_copy, ";");
    char *packed_access_key = packed_connection_string_copy;

    const char *host = chirp_get_host_name(packed_host_name);
    const char *device = chirp_get_device_id(packed_device_id);
    const char *key = chirp_get_access_key(packed_access_key);

    size_t connection_string_length = strlen(host) + strlen(device) + strlen(key) + 2;
    auto *connection_string = (char *) calloc(connection_string_length + 1, sizeof(char));  

    strcat(connection_string, host);
    strcat(connection_string, ";");
    strcat(connection_string, device);
    strcat(connection_string, ";");
    strcat(connection_string, key);
    strcat(connection_string, "\0");

    free((void*)host);
    free((void*)device);
    free((void*)key);
    free(save_ptr);

    return connection_string;
}

void on_receiving_callback(void *data, uint8_t *payload, size_t data_length, uint8_t channel)
{
    rgbLed.setColor(255, 255, 255);
    free(payload);
}

// This callback is reached when the Chirp SDK decodes a payload
void on_received_callback(void *ptr, uint8_t *data, size_t length, uint8_t channel)
{
    rgbLed.setColor(0, 0, 0);

    if (data != nullptr && length != 0)
    {
        Screen.clean();
        Screen.print(0, "Data received");

        auto *message = (char *) calloc(length + 1, sizeof(char));

        // decrypt the data
        for (int i = 0; i < length; i++) {
            data[i] = data[i] ^ key[i % 6];
        }
        strncpy(message, (const char *) data, length);

        char *save_message_pointer = message;

        //char *magicNumber = strsep(&message, ":");
        char *ssid = strsep(&message, ":");
        char *pwd = strsep(&message, ":");
        char *packed_connection_string = message;
        char *connection_string = chirp_reconstruct_connection_string(packed_connection_string);

        memcpy(chirp_ssid, ssid, 64);
        memcpy(chirp_pwd, pwd, 64);
        memcpy(chirp_connection_string, connection_string, 256);

        free(save_message_pointer);
        free(packed_connection_string);
        free(connection_string);

        chirp_decode_ok = true;
        chirp_heard = true;
    } else {
        Screen.clean();
        Screen.print(0, "Decode failed");
        Screen.print(1, "Please adjust");
        Screen.print(2, "the volume and");
        Screen.print(3, "try again");
    }
}

void recordCallback(void)
{
    Audio.readFromRecordBuffer((char *) shortDecoderBuffer, AUDIO_CHUNK_SIZE);
    decoderBufferState = BUFFER_STATE_FULL;
}

void error_handler(chirp_connect_error_code_t err)
{
    if (err != CHIRP_CONNECT_OK)
    {
        Serial.printf("%s\n", chirp_connect_error_code_to_string(err));
        while(true);
    }
}

// standard  Arduino setup function - called once whent he device initializes on power up
void setup()
{
    // set the serial baud rate
	Serial.begin(115200);

    // read ssid and password if empty assume not configured
    char ssid[64];
    char password[64];
    readWiFi(ssid, 64, password, 64);

    if (strlen(ssid) == 0 && strlen(password) == 0) {
        chirpConnect = new_chirp_connect(APP_KEY, APP_SECRET);
        if (chirpConnect)
        {
            printf("Chirp SDK initialized\n");
        }
        else
        {
            printf("Chirp SDK initialization failed\n");
            exit(1);
        }

        chirp_connect_error_code_t err = chirp_connect_set_config(chirpConnect, APP_LICENCE);
        error_handler(err);

        printf("Licence set correctly\n");

        char *info = chirp_connect_get_info(chirpConnect);
        printf("%s\n", info);
        free(info);

        err = chirp_connect_set_sample_rate(chirpConnect, SAMPLE_RATE);
        error_handler(err);
        printf("New sample rate is : %d\n", chirp_connect_get_sample_rate(chirpConnect));

        chirp_connect_callback_set_t callbacks = {0};
        callbacks.on_received = on_received_callback;
        callbacks.on_receiving = on_receiving_callback;

        err = chirp_connect_set_callbacks(chirpConnect, callbacks);
        error_handler(err);

        printf("Callbacks set\n");

        strcpy(key, "");

        // init i2c for reading sensors
        i2c = new DevI2C(D14, D15);

        // seed the pseudo-random number generator using magnetic field as the seed
        LIS2MDLSensor *lis2mdl = new LIS2MDLSensor(*i2c);
        lis2mdl->init(NULL);
        int axes[3];
        lis2mdl->getMAxes(axes);
        delete lis2mdl;
        randomSeed((int)((axes[0] * axes[1] * axes[2]) / 3));

        // create a random six digit PIN
        for (int i = 0; i < 6; i++)
            key[i] = '0' + random(0, 10);

        Screen.clean();
        char buf[12];
        sprintf(buf, "PIN: %s", key);
        Screen.print(0, buf);
        Screen.print(2, "Listening ...");

        // needed to correct the MXChip audio clock with an offset
        chirp_connect_set_frequency_correction(chirpConnect, 0.9950933459f);

        err = chirp_connect_start(chirpConnect);
        error_handler(err);

        Audio.format(SAMPLE_RATE, AUDIO_SAMPLE_SIZE);
        Audio.startRecord(recordCallback);

        while(!chirp_heard)
        {
            if (decoderBufferState == BUFFER_STATE_FULL)
            {
                for (uint16_t i = 0; i < AUDIO_CHUNK_SIZE / 4; i++)
                floatDecoderBuffer[i] = (float) (shortDecoderBuffer[i * 2] / 32767.0f);

                chirp_connect_process_input(chirpConnect, floatDecoderBuffer, AUDIO_CHUNK_SIZE / 4);  
                decoderBufferState = BUFFER_STATE_EMPTY;
             }
        }

        Audio.stop();
        chirp_connect_stop(chirpConnect);
        del_chirp_connect(&chirpConnect);        

        if(chirp_decode_ok)
        {
            Serial.printf("MXChip : SSID = \"%s\"\n", chirp_ssid);
            Serial.printf("MXChip : PWD = \"%s\"\n", chirp_pwd);  
            Serial.printf("MXChip : CON_STR = \"%s\"\n\n", chirp_connection_string);  
            
            storeWiFi(chirp_ssid, chirp_pwd);
            storeConnectionString(chirp_connection_string);
        } else {
            Serial.printf("MXChip failed to be initialized with IoT Hub credentials, retry sending Chirp");
        }

        readWiFi(ssid, 64, password, 64); 
    }

    configured = true;

    // init the status LED's
    pinMode(LED_WIFI, OUTPUT);
    pinMode(LED_AZURE, OUTPUT);
    pinMode(LED_USER, OUTPUT);

    Screen.clean();

    // start the wifi
    if(WiFi.begin(ssid, password) == WL_CONNECTED) {
        digitalWrite(LED_WIFI, 1);
        connected = true;
        Screen.print(0, "wifi connected");
        unsigned char mac[6];
        WiFi.macAddress(mac);
        char macStr[12];
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        (void)Serial.printf("MAC: %s\r\n", macStr);
    }

    // initialize the platform
    if (platform_init() != 0) {
        (void)Serial.printf("Failed to initialize the platform.\r\n");
        return;
    }

    // connect to the hub
    String myConnStr = readConnectionString();
    if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(myConnStr.c_str(), MQTT_Protocol)) == NULL) {
        (void)Serial.printf("ERROR: iotHubClientHandle is NULL!\r\n");
        return;
    } else {
        Screen.print(0, "Connected to hub");
    }

    // set some options
    IoTHubClient_LL_SetRetryPolicy(iotHubClientHandle, IOTHUB_CLIENT_RETRY_EXPONENTIAL_BACKOFF, 1200);
    IoTHubClient_LL_SetOption(iotHubClientHandle, "logtrace", &traceOn);
    if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK) {
        (void)Serial.printf("Failed to set option \"TrustedCerts\"\r\n");
        return;
    }

    // Setting twin call back, so we can receive desired properties. 
    if (IoTHubClient_LL_SetDeviceTwinCallback(iotHubClientHandle, deviceTwinGetStateCallback, NULL) != IOTHUB_CLIENT_OK) {
        (void)Serial.printf("ERROR: IoTHubClient_LL_SetDeviceTwinCallback..........FAILED!\r\n");
        return;
    }

    // init sensors
    tempHumidity = new HTS221Sensor(*i2c);
    tempHumidity->init(NULL);

    pressure = new LPS22HBSensor(*i2c);
    pressure->init(NULL);

    // init buttons as input
    pinMode(USER_BUTTON_A, INPUT);
    pinMode(USER_BUTTON_B, INPUT);
}

// callback to process telemetry acknowledgments from the hub
static void sendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback) {
    EVENT_INSTANCE *eventInstance = (EVENT_INSTANCE *)userContextCallback;

    (void)Serial.printf("Confirmation received for message tracking_id = %d with result = %s\r\n", eventInstance->messageTrackingId, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    if (result == IOTHUB_CLIENT_CONFIRMATION_OK) {
        ackCount++;
    } else {
        errorCount++;
    }
    
    IoTHubMessage_Destroy(eventInstance->messageHandle);
    free(eventInstance);
}

// send a telemetry payload to the IoT Hub
bool sendTelemetry(const char *payload) {  
    IOTHUB_CLIENT_RESULT hubResult = IOTHUB_CLIENT_RESULT::IOTHUB_CLIENT_OK;

    // build the message from the passed in payload
    EVENT_INSTANCE *currentMessage = (EVENT_INSTANCE*)malloc(sizeof(EVENT_INSTANCE));
    currentMessage->messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)payload, strlen(payload));
    if (currentMessage->messageHandle == NULL) {
        (void)Serial.printf("ERROR: iotHubMessageHandle is NULL!\r\n");
        free(currentMessage);
        return false;
    }
    // add in the tracking id
    currentMessage->messageTrackingId = trackingId++;

    MAP_HANDLE propMap = IoTHubMessage_Properties(currentMessage->messageHandle);

    // add a timestamp to the message - illustrated for the use in batching
    time_t seconds = time(NULL);
    String temp = ctime(&seconds);
    temp.replace("\n","\0");
    if (Map_AddOrUpdate(propMap, "timestamp", temp.c_str()) != MAP_OK)
    {
        Serial.println("ERROR: Adding message property failed");
    }
    
    // submit the message to the Azure IoT hub
    hubResult = IoTHubClient_LL_SendEventAsync(iotHubClientHandle, currentMessage->messageHandle, sendConfirmationCallback, currentMessage);
    if (hubResult != IOTHUB_CLIENT_OK) {
        (void)Serial.printf("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED (%s)!\r\n", hubResult);
        errorCount++;
        IoTHubMessage_Destroy(currentMessage->messageHandle);
        free(currentMessage);
        return false;
    } else {
        Serial.printf("IoTHubClient_LL_SendEventAsync accepted message for transmission to IoT Hub with tracking_id = %d\r\n", currentMessage->messageTrackingId);
    }

    // flash the Azure LED
    digitalWrite(LED_AZURE, 1);
    delay(500);
    digitalWrite(LED_AZURE, 0);

    return true;
}

// read temperature sensor
float readTempSensor() {
    float tempValue;
    tempHumidity->reset();
    if (tempHumidity->getTemperature(&tempValue) == 0)
        return tempValue;
    else
        return 0xFFFF;
}

// read humidity sensor
float readHumiditySensor() {
    float humidityValue;
    tempHumidity->reset();
    if (tempHumidity->getHumidity(&humidityValue) == 0)
        return humidityValue;
    else
        return 0xFFFF;
}

// read pressure sensor
float readPressureSensor() {
    float pressureValue;
    if (pressure->getPressure(&pressureValue) == 0)
        return pressureValue;
    else
        return 0xFFFF;
}

// standard  Arduino loop function - called repeatedly for ever, think of this as the 
// event loop or message pump.  Try not to block this for long periods of time as your 
// code is single threaded.
void loop()
{
    if (configured) {
        // Send telemetry every 5 seconds
        if (millis() - lastTelemetrySend >= 5000) {
            // read sensors
            float temp = readTempSensor();
            float humidity = readHumiditySensor();
            float pressure = readPressureSensor();

            // build the JSON payload
            char payload[255];
            sprintf(payload, 
                    "{\"humidity\": %f, \"temp\": %f, \"pressure\":%f, \"magnetometerX\": %d, \"magnetometerY\": %d, \"magnetometerZ\": %d, \"accelerometerX\": %d, \"accelerometerY\": %d, \"accelerometerZ\": %d, \"gyroscopeX\": %d, \"gyroscopeY\": %d, \"gyroscopeZ\": %d}",
                    humidity, temp, pressure, RSV(), RSV(), RSV(), RSV(), RSV(), RSV(), RSV(), RSV(), RSV()
            );
            if (payloadLogging) {
                Serial.println(payload);
            }

            // send the telemetry
            if (sendTelemetry(payload)) {
                (void)Serial.printf("Send telemetry success\r\n");
                sentCount++;
            } else {
                (void)Serial.printf("Failed to send telemetry\r\n");
                errorCount++;
            }

            // display the send/ack/error stats on the display
            char buff[64];
            sprintf(buff, "sent: %d\r\nack: %d\r\nerror: %d", sentCount, ackCount, errorCount);
            Screen.print(0, buff);

            lastTelemetrySend = millis();
        }

        // if buttons A and B are pressed and held down together the device 
        // will reset the WiFi and connection string values
        if(digitalRead(USER_BUTTON_A) == LOW && digitalRead(USER_BUTTON_B) == LOW) {
            Screen.clean();
            Screen.print(0, "Resetting ...");

            // clear values from EEPROM
            clearWiFi();
            clearConnectionString();

            // reboot the device
            SystemReboot();
        }

        // Send an event when the user presses the A button
        if(digitalRead(USER_BUTTON_A) == LOW && (millis() - lastSwitchPress > switchDebounceTime)) {
            Serial.println("Button A pressed");

            // flash the user LED
            digitalWrite(LED_USER, 1);
            delay(500);
            digitalWrite(LED_USER, 0);

            // send the event
            char buff[1024];
            // get current time
            time_t seconds = time(NULL);
            String temp = ctime(&seconds);
            temp.replace("\n","\0");

            // build the event payload
            sprintf(buff, "{\"buttonA\": \"%s\"}", temp.c_str());
            if (payloadLogging) {
                Serial.println(buff);
            }

            // send the event - it's just a telemetry message
            if (sendTelemetry(buff)) {
                (void)Serial.printf("Send telemetry success\r\n");
                sentCount++;
            } else {
                (void)Serial.printf("Failed to send telemetry\r\n");
                errorCount++;
            }

            lastSwitchPress = millis();
        }

        // Send the reported property DieNumber when the user presses the B button
        if(digitalRead(USER_BUTTON_B) == LOW && (millis() - lastSwitchPress > switchDebounceTime)) {
            Serial.println("Button B pressed");

            // flash the user LED
            digitalWrite(LED_USER, 1);
            delay(500);
            digitalWrite(LED_USER, 0);

            // generate the die roll
            int dieNumber = randVal(1, 6);
            Serial.printf("You rolled a %d\r\n", dieNumber);

            // send the reported property
            char buff[1024];

            // build the reported property JSON payload
            sprintf(buff, "{\"dieNumber\": %d}", dieNumber);
            if (payloadLogging) {
                Serial.println(buff);
            }

            // send the reported property
            sendReportedProperty(buff);

            lastSwitchPress = millis();
        }

        // yield to process any work to/from the hub
        hubClientYield();
    }

    delay(1);  // need a minimum delay for stability
}
