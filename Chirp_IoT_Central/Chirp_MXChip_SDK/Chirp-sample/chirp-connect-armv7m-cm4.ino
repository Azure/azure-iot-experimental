/**------------------------------------------------------------------------------
 *
 *  Simple example of the Chirp Connect C SDK on the Microsoft MXChip IoT DevKit.
 *
 *  @file chirp-connect-armv7m-cm4.ino
 * 
 *  @brief After having created an account on https://developers.chirp.io, get 
 *  your Key, Secret and Config string from your account using the Microsoft-MXChip
 *  protocol and set them a few lines below in this file.
 * 
 *  This example will start in listening mode. The listening and playing modes
 *  can alternate by pressing the button A (left one).
 * 
 *  Each time the SDK will start receiving some data the LED will turn blue.
 *  If the data has been sucessfully decoded then the LED will turn green and the
 *  hexadecimal representation of the data as well as the length of the message,
 *  in bytes, will be displayed. If the decode failed the LED will turn red.
 * 
 *  In playing mode, each push on the button B (right one) will start sending a
 *  random payload of random lenght and turn the LED yellow. Once the payload
 *  has been sent the LED wil turn cyan and the hexadecimal representation of the
 *  data sent as well as the length of the payload, in bytes, will be displayed.
 *  The audio data is sent by the 3.5 mm jack output.
 * 
 *  Known issues :
 *  - For some reason, at 16kHz, recording again after having played almost always
 *    fails. The quick and easy solution is to restart the program by presing the
 *    reset button.
 *
 *  Copyright © 2011-2018, Asio Ltd.
 *  All rights reserved.
 *
 *----------------------------------------------------------------------------*/

/*
 * MXChip and Arduino native headers.
 */ 
#include "Arduino.h"
#include "AudioClassV2.h"
#include "OledDisplay.h"
#include "RGB_LED.h"

/*
 * Main Chirp Connect header. This header and the ones it depends on must be in
 * the same folder.
 */ 
#include "chirp_connect.h"

/* 
 * The audio sample rate used by the microphone. So far it's the only one
 * supported by the SDK on this board.
 */
#define SAMPLE_RATE 16000
#define AUDIO_SAMPLE_SIZE 16
#define SHORT_BUFFER_SIZE (AUDIO_CHUNK_SIZE / 2)
#define FLOAT_BUFFER_SIZE (SHORT_BUFFER_SIZE / 2)

/*
 * Set the following defines with the key, secret and config string coming from
 * your Chirp Developer account.
 */
#define APP_KEY "Your_chirp_account_key"
#define APP_SECRET "Your_chirp_account_secret"
#define APP_CONFIG "Your_chirp_account_config_string" 

/* 
 * Allows to keep track of the state of some audio buffers.
 */
typedef enum {
  BUFFER_STATE_NONE,
  BUFFER_STATE_EMPTY,
  BUFFER_STATE_FULL,
} bufferState;

/* 
 * Class handling the audio on the board. The state is recording by default.
 */
static AudioClass& Audio = AudioClass::getInstance();
AUDIO_STATE_TypeDef audioState = AUDIO_STATE_RECORDING;

/* 
 * Buffers containnig the audio to play and record data.
 */
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

/* 
 * Global pointer to the SDK structure. This is global as this pointer is
 * needed when processing the audio in the loop() function.
 */
chirp_connect_t *chirpConnect = NULL;

/* 
 * Simple error handler which display an error message and loop indefinitely.
 */
void errorHandler(chirp_connect_error_code_t errorCode)
{
    if (errorCode != CHIRP_CONNECT_OK)
    {
        Serial.printf("Error handler : %s\n", chirp_connect_error_code_to_string(errorCode));
        while(true);
    }
}

/* 
 * Audio recording callback called by the Audio Class instance when a new
 * buffer of samples is available with new recorded samples.
 */
void recordCallback(void)
{
    Audio.readFromRecordBuffer((char *) shortRecordBuffer, SHORT_BUFFER_SIZE * 2);
    recordBufferState = BUFFER_STATE_FULL;
}

/* 
 * Audio playing callback called by the Audio Class instance when a new
 * buffer of samples is available with new samples to be played.
 */
void playCallback(void)
{
    if(playBufferState == BUFFER_STATE_FULL)
    {
        Audio.writeToPlayBuffer((char *) shortPlayBuffer, SHORT_BUFFER_SIZE * 2);
        playBufferState = BUFFER_STATE_EMPTY;
    }
}

/* 
 * Callback reached when the SDK starts sending data.
 */
void on_sending_callback(void *data, uint8_t *payload, size_t length, uint8_t channel)
{
    Screen.clean();
    rgbLed.setColor(255, 255, 0);
}

/* 
 * Callback reached when the SDK has sent the data.
 */
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

/* 
 * Callback reached when the SDK starts receiving some data.
 */
void on_receiving_callback(void *data, uint8_t *payload, size_t length, uint8_t channel)
{
    rgbLed.setColor(0, 0, 255);
}

/* 
 * Callback reached when the SDK has received some data.
 */
void on_received_callback(void *data, uint8_t *payload, size_t length, uint8_t channel)
{
    // A pointer not null with a length different than 0 means the decode has succedeed.
    if (payload && length != 0)
    {
        char *identifier = chirp_connect_as_string(chirpConnect, payload, length);
        char strLength[8] = {0};
        itoa(length, strLength, 10);

        Screen.clean();
        Screen.print(0, "Received !");
        Screen.print(1, (const char *) identifier, true);
        Screen.print(3, strLength);
        rgbLed.setColor(0, 255, 0);

        free(payload);
        free(identifier);
    }
    // A null pointer with a length of 0 means the decode has failed.
    else
    {
        Screen.clean();
        Screen.print(0, "Decode failed =(");  
        rgbLed.setColor(255, 0, 0);
    }
}

/* 
 * Setup function where the SDK is initialised.
 */
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
        printf("Initialisation failed\nAre your key and secret correct ?\n");
        exit(1);
    }

    chirp_connect_error_code_t errorCode = chirp_connect_set_config(chirpConnect, APP_CONFIG);
    errorHandler(errorCode);

    printf("Licence set correctly\n");

    char *info = chirp_connect_get_info(chirpConnect);
    printf("%s - V%s\n", info, chirp_connect_get_version());
    free(info);

    errorCode = chirp_connect_set_sample_rate(chirpConnect, SAMPLE_RATE);
    errorHandler(errorCode);
    
    printf("Sample rate is : %u\n", chirp_connect_get_sample_rate(chirpConnect));

    // The static structure is set to 0. This is needed because we are not setting
    // the `on_state_changed` callback.
    chirp_connect_callback_set_t callbacks = {0};
    callbacks.on_sending = on_sending_callback;
    callbacks.on_sent = on_sent_callback;
    callbacks.on_received = on_received_callback;
    callbacks.on_receiving = on_receiving_callback;

    errorCode = chirp_connect_set_callbacks(chirpConnect, callbacks);
    errorHandler(errorCode);

    printf("Callbacks set\n");

    // MXChip specific : A software adjustment of the sample rate is needed.
    errorCode = chirp_connect_set_frequency_correction(chirpConnect, 0.9950933459f);
    errorHandler(errorCode);

    errorCode = chirp_connect_start(chirpConnect);
    errorHandler(errorCode);
    
    printf("SDK started\n");

    Screen.clean();
    Screen.print(0, "Chirp C SDK");
    Screen.print(1, "Listening ...");

    // Setup the audio class and start recording.
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

    //  If we've pressed the button A, alternate the audio state between Recording and Playing.
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
            Audio.startRecord(recordCallback);
            Screen.clean();
            Screen.print(0, "Recording");
        }

        rgbLed.turnOff();
    }

    if (audioState == AUDIO_STATE_RECORDING)
    {
        // Once the recording buffer is full, we process it.
        if (recordBufferState == BUFFER_STATE_FULL)
        {
            // Convert the stereo audio samples into mono by taking every other sample and convert them
            // from int16_t to float.
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
        // If the button B is pressed a chirp is sent.
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
            // On the contrary of the recording part, we duplicate the data produced by the SDK
            // to create a stereo audio stream that is converted from float to int16_t samples.
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
