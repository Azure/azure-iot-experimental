#include "Arduino.h"

// EEPROM
#include "EEPROMInterface.h"

#define IOT_CENTRAL_ZONE_IDX      0x02
#define IOT_CENTRAL_MAX_LEN       128

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

// standard  Arduino setup function - called once whent he device initializes on power up
void setup() {
    Screen.clean();
    Screen.print(0, "Resetting EEPROM");

    // clear values from EEPROM
    clearWiFi();
    clearConnectionString();

    Screen.clean();
    Screen.print(0, "EEPROM cleared");
}

// standard  Arduino loop function - called repeatedly for ever, think of this as the 
// event loop or message pump.  Try not to block this for long periods of time as your 
// code is single threaded.
void loop() {
    delay(1);
}