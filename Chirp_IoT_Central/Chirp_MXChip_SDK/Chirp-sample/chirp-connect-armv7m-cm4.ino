#include "Arduino.h"
#include "AudioClassV2.h"
#include "OledDisplay.h"
#include "RGB_LED.h"

#include "chirp_connect.h"

#define SAMPLE_RATE 16000
#define AUDIO_SAMPLE_SIZE 16
#define SHORT_BUFFER_SIZE (AUDIO_CHUNK_SIZE / 2)
#define FLOAT_BUFFER_SIZE (SHORT_BUFFER_SIZE / 2)

// The 3 following fields will be filled by the user with they credentials coming from their Chirp account
#define APP_KEY "995Cd69Da64246F1Ac223F1cd"
#define APP_SECRET "6d3dE8B9b4383bCAdF8Fe252479e7cBBceB61c05b34edb46da"
#define APP_LICENCE "hOiB7ISfu5JIHUXK0dh7qdbUbYugUAnZlCcdY7r/XwV1Zwqf0ueWJ5QOTElgW3Sh/LkAg+Bgapsvmznc+ET0lUW/OSKBj/mmm+51H5RJ62SUb/TfszZ6eO0QIGlsOShk3+ANA1JeVFaYIZSIKdCCAzVEjIvZJjPw9t9sRG+bf1Zjmvklfmoi4A+SAjBXtlZqYCEjw8un32AMT0MQfqg8oBZowQYGcb8diCEyocdmgqrLzaKivDuxaTFspb+P0/ciZxgZUzRRdzPt9/+RIXGiLOJgX208TkqahWxUsSsbHDjHsoKQxk5p8r5w4Z3OcRRLKt4ZIHeQHgQo5j9JjWO726fx2SqCyNj+Q02YVYzis0evT6Q9oX/3upZ0mcRS/z6W2H7/zmy+tEYfNJKnAQ3LK1DGWbuJS9k3etWhX993KCZqx43pOP3aymtDopv9AcpbdM/bkc3ANuglqOdwnOCa/18Ixu5b+e/Zfd+PeyYkxmMPsA+1zy9Ki1Be6L6QVz+rSgVhzSzNP1e4cjuMG6g3Oe2/pm2Fmvd1dZbWEDTlxXyFF/Ap8Lcikkx2zxoH/Mpe82GM/Y/n4s9vwR4RlpBUfbAcNxpZLGygrsWxVOXidIk55eXXPjyuB5xZqyZF9tpM8KMkA+bvNmstiRO+fz1Ab+IJ42xsTDU+yFu+L+CQhx2F6MMUQ+MGNi2Lou0Rie2c+pM8qVSqpg1MBd67bQHJnkv0ltnln35b7Pbrxy5j5CXsE2aA0kdPqhpEE9Ynq4PBk1orCAWxt7jMGzx7PtLOPkVZn2U3x9K4kz4iKBM3S9PEy0k8eimqBB4pHIzKACdRwmzQtSHyGPcMucMzuz2iz6udjr129ZGmVMQ6uCM/NEVd99hO9Iv/bBijYyWCd+pRKP5jdl0LdhoZHSBiHNvTB2oYfXbpQSgTjLUl6qc0w7t2Tj/yDnwROmotxK1bJhj3Cl/8mHNmbOM77hzFmaB/4MI+tMhdcZpqj9qtEF0lfQcUQJ2xB/R3B10siOhTJMkhIvrejzxKjzicPSl62chEJ9NUdZvWbJm6wEy+ExR/fNM=" 

typedef enum {
  BUFFER_STATE_NONE,
  BUFFER_STATE_EMPTY,
  BUFFER_STATE_FULL,
} bufferState;

static AudioClass& Audio = AudioClass::getInstance();
AUDIO_STATE_TypeDef audioState = AUDIO_STATE_RECORDING;

static int16_t shortRecordBuffer[SHORT_BUFFER_SIZE] = {0};
static float floatRecordBuffer[FLOAT_BUFFER_SIZE] = {0};
bufferState recordBufferState = BUFFER_STATE_EMPTY;

static int16_t shortPlayBuffer[SHORT_BUFFER_SIZE] = {0};
bufferState playBufferState = BUFFER_STATE_EMPTY;

int lastButtonAState;
int buttonAState;

int buttonBState;
int lastButtonBState;

RGB_LED rgbLed;

chirp_connect_t *chirpConnect = NULL;

void errorHandler(chirp_connect_error_code_t errorCode)
{
    if (errorCode != CHIRP_CONNECT_OK)
    {
        Serial.printf("%s\n", chirp_connect_error_code_to_string(errorCode));
        while(true);
    }
}

void recordCallback(void)
{
    Audio.readFromRecordBuffer((char *) shortRecordBuffer, SHORT_BUFFER_SIZE * 2);
    recordBufferState = BUFFER_STATE_FULL;
}

void playCallback(void)
{
    if(playBufferState == BUFFER_STATE_FULL)
    {
        Audio.writeToPlayBuffer((char *) shortPlayBuffer, SHORT_BUFFER_SIZE * 2);
        playBufferState = BUFFER_STATE_EMPTY;
    }
}

void on_sending_callback(void *data, uint8_t *payload, size_t length, uint8_t channel)
{
    char *identifier = chirp_connect_as_string(chirpConnect, payload, length);
    char strLength[8] = {0};
    itoa(length, strLength, 10);

    Screen.clean();
    Screen.print(0, "Sending !");
    Screen.print(1, (const char *) identifier, true);
    Screen.print(3, strLength);
    rgbLed.setColor(255, 255, 0);

    free(payload);
    free(identifier);
}

void on_sent_callback(void *data, uint8_t *payload, size_t length, uint8_t channel)
{
    char *identifier = chirp_connect_as_string(chirpConnect, payload, length);
    char strLength[8] = {0};
    itoa(length, strLength, 10);

    Screen.clean();
    Screen.print(0, "Sent !");
    Screen.print(1, (const char *) identifier, true);
    Screen.print(3, strLength);
    rgbLed.setColor(0, 255, 255);

    free(payload);
    free(identifier);
}

void on_receiving_callback(void *data, uint8_t *payload, size_t length, uint8_t channel)
{
    rgbLed.setColor(0, 255, 255);
}

void on_received_callback(void *data, uint8_t *payload, size_t length, uint8_t channel)
{
    if (payload && length != 0)
    {
        char *identifier = chirp_connect_as_string(chirpConnect, payload, length);
        char strLength[8] = {0};
        itoa(length, strLength, 10);

        Screen.clean();
        Screen.print(0, "Received !");
        Screen.print(1, (const char *) identifier, true);
        Screen.print(3, strLength);
        rgbLed.setColor(0, 0, 255);

        free(payload);
        free(identifier);
    }
    else
    {
        Screen.clean();
        Screen.print(0, "Decode failed =(");  
        rgbLed.setColor(255, 0, 0);
    }
}

void setup()
{
	Serial.begin(115200);
    delay(1000);

    pinMode(USER_BUTTON_A, INPUT);
    lastButtonAState = digitalRead(USER_BUTTON_A);

    chirpConnect = new_chirp_connect(APP_KEY, APP_SECRET);
    if (chirpConnect)
    {
        printf("Initialisation is OK\n");
    }
    else
    {
        printf("Initialisation failed\n");
        exit(1);
    }

    chirp_connect_error_code_t errorCode = chirp_connect_set_licence(chirpConnect, APP_LICENCE);
    errorHandler(errorCode);

    printf("Licence set correctly\n");

    char *info = chirp_connect_get_info(chirpConnect);
    Serial.printf("%s\n", info);
    free(info);

    errorCode = chirp_connect_set_sample_rate(chirpConnect, SAMPLE_RATE);
    errorHandler(errorCode);
    
    printf("Sample rate is : %u\n", chirp_connect_get_sample_rate(chirpConnect));

    chirp_connect_callback_set_t callbacks = {0};
    callbacks.on_sending = on_sending_callback;
    callbacks.on_sent = on_sent_callback;
    callbacks.on_received = on_received_callback;
    callbacks.on_receiving = on_receiving_callback;

    errorCode = chirp_connect_set_callbacks(chirpConnect, callbacks);
    errorHandler(errorCode);

    printf("Callbacks set\n");

    errorCode = chirp_connect_start(chirpConnect);
    errorHandler(errorCode);

    printf("SDK started\n");

    Screen.clean();
    Screen.print(0, "Chirp SDK");
    Screen.print(1, "Recording");

    Audio.format(SAMPLE_RATE, AUDIO_SAMPLE_SIZE);
    int res = Audio.startRecord(recordCallback);
    if (res != 0)
    {
        Serial.printf("Error when starting audio\n");
        while(true);
    }
}

void loop()
{
    buttonAState = digitalRead(USER_BUTTON_A);
    buttonBState = digitalRead(USER_BUTTON_B);
    chirp_connect_error_code_t errorCode;

    if (buttonAState == LOW && lastButtonAState == HIGH)
    {
        if (audioState == AUDIO_STATE_RECORDING)
        {
            audioState = AUDIO_STATE_PLAYING;
            Audio.stop();
            Audio.startPlay(playCallback);
            Screen.clean();
            Screen.print(0, "Playing");
        }
        else if (audioState == AUDIO_STATE_PLAYING)
        {
            audioState = AUDIO_STATE_RECORDING;
            Audio.stop();
            // For some reason, at 16kHz, recording again after having played doesn't always work.
            Audio.startRecord(recordCallback);
            Screen.clean();
            Screen.print(0, "Recording");
        }

        rgbLed.turnOff();
    }

    if (audioState == AUDIO_STATE_RECORDING)
    {
        if (recordBufferState == BUFFER_STATE_FULL)
        {
            for (int i = 0; i < FLOAT_BUFFER_SIZE; i++)
            {
                floatRecordBuffer[i] = (float) shortRecordBuffer[i * 2] / 32767.0f;
            }
            errorCode = chirp_connect_process_input(chirpConnect, floatRecordBuffer, FLOAT_BUFFER_SIZE);
            errorHandler(errorCode);
            recordBufferState = BUFFER_STATE_EMPTY;
        }
    }
    else if (audioState == AUDIO_STATE_PLAYING)
    {
        if (buttonBState == LOW && lastButtonBState == HIGH)
        {
            size_t randomPayloadLength = 0;
            uint8_t *randomPayload =  chirp_connect_random_payload(chirpConnect, &randomPayloadLength);
            errorCode = chirp_connect_send(chirpConnect, randomPayload, randomPayloadLength);
            errorHandler(errorCode);
            free(randomPayload);
        }

        chirp_connect_state_t state = chirp_connect_get_state(chirpConnect);
        if (state == CHIRP_CONNECT_STATE_SENDING && playBufferState == BUFFER_STATE_EMPTY)
        {
            float tmpBuffer[SHORT_BUFFER_SIZE / 2] = {0};
            errorCode = chirp_connect_process_output(chirpConnect, tmpBuffer, SHORT_BUFFER_SIZE / 2);
            errorHandler(errorCode);
            for (uint16_t i = 0; i < SHORT_BUFFER_SIZE / 2; i++)
            {
                shortPlayBuffer[i * 2] = tmpBuffer[i] * 32767.0f;
                shortPlayBuffer[i * 2 + 1] = tmpBuffer[i] * 32767.0f;
            }
            playBufferState = BUFFER_STATE_FULL;
        }
    }

    lastButtonAState = buttonAState;
    lastButtonBState = buttonBState;
}
