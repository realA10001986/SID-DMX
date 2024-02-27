/*
 * -------------------------------------------------------------------
 * CircuitSetup.us SID - DMX-controlled
 * (C) 2024 Thomas Winischhofer (A10001986)
 * All rights reserved.
 * -------------------------------------------------------------------
 */

#include "sid_global.h"

/*
#define ARDUINOJSON_USE_LONG_LONG 0
#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_STD_STREAM 0
#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_NAN 0
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
*/
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#ifdef USE_SPIFFS
#include <SPIFFS.h>
#else
#define SPIFFS LittleFS
#include <LittleFS.h>
#endif

#include <Update.h>

#include "sid_settings.h"
#include "sid_dmx.h"

// Size of main config JSON
// Needs to be adapted when config grows
#define JSON_SIZE 2500
#if ARDUINOJSON_VERSION_MAJOR >= 7
#define DECLARE_S_JSON(x,n) JsonDocument n;
#define DECLARE_D_JSON(x,n) JsonDocument n;
#else
#define DECLARE_S_JSON(x,n) StaticJsonDocument<x> n;
#define DECLARE_D_JSON(x,n) DynamicJsonDocument n(x);
#endif 

static const char *fwfn = "/sidfw.bin";    //"/sid-DMX.ino.nodemcu-32s.bin";
static const char *fwfnold = "/sidfw.old"; //"/sid-DMX.ino.nodemcu-32s.old";

static const char *fsNoAvail = "File System not available";
static const char *failFileWrite = "Failed to open file for writing";
#ifdef SID_DBG
static const char *badConfig = "Settings bad/missing/incomplete; writing new file";
#endif

/* If SPIFFS/LittleFS is mounted */
bool haveFS = false;

/* If a SD card is found */
bool haveSD = false;

/*
static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, const char *funcName);
static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, const char *funcName);
*/

static bool firmware_update();

/*
 * settings_setup()
 * 
 * Mount SPIFFS/LittleFS and SD (if available).
 * Read configuration from JSON config file
 * If config file not found, create one with default settings
 *
 * If the device is powered on or reset while ENTER is held down, 
 * the IP settings file will be deleted and the device will use DHCP.
 */
void settings_setup()
{
    const char *funcName = "settings_setup";
    bool SDres = false;

    #ifdef SID_DBG
    Serial.printf("%s: Mounting flash FS... ", funcName);
    #endif

    if(SPIFFS.begin()) {

        haveFS = true;

    } else {

        #ifdef SID_DBG
        Serial.print(F("failed, formatting... "));
        #endif

        SPIFFS.format();
        if(SPIFFS.begin()) haveFS = true;

    }
    
    // Set up SD card
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    haveSD = false;
    
    #ifdef SID_DBG
    Serial.printf("%s: Mounting SD... ", funcName);
    #endif

    if(!(SDres = SD.begin(SD_CS_PIN, SPI, 16000000))) {
        #ifdef SID_DBG
        Serial.printf("Retrying at 25Mhz... ");
        #endif
        SDres = SD.begin(SD_CS_PIN, SPI, 25000000);
    }

    if(SDres) {

        #ifdef SID_DBG
        Serial.println(F("ok"));
        #endif

        uint8_t cardType = SD.cardType();
       
        #ifdef SID_DBG
        const char *sdTypes[5] = { "No card", "MMC", "SD", "SDHC", "unknown (SD not usable)" };
        Serial.printf("SD card type: %s\n", sdTypes[cardType > 4 ? 4 : cardType]);
        #endif

        haveSD = ((cardType != CARD_NONE) && (cardType != CARD_UNKNOWN));

    } else {

        Serial.println(F("No SD card found"));

    }

    if(haveSD) {
        if(SD.exists(fwfn)) {
            showWaitSequence();
            if(!firmware_update()) {
                showCopyError();
                delay(5000);
            }
        }
    }
}


static bool firmware_update()
{
    uint32_t maxSketchSpace = UPDATE_SIZE_UNKNOWN;
    uint8_t  buf[1024];
    bool     error = false;
    size_t   s;
    
    File myFile = SD.open(fwfn, FILE_READ);
    
    if(!myFile) {
        Serial.println("Failed to open firmware file");
        return false;
    }
    
    if(!Update.begin(maxSketchSpace)) {
        
        Serial.printf("Firmware update error %d\n", Update.getError());
        
        Update.end();
        return false;
    }

    while((s = myFile.read(buf, 1024))) {
        if(Update.write(buf, s) != s) {
            Serial.printf("Firmware update write error %d\n", Update.getError());
            error = true;
            break;
        }
    }

    if(!error) {
        Update.end(true);
        if(!Update.hasError()) {
            SD.remove(fwfnold);
            SD.rename(fwfn, fwfnold);
            unmount_fs();
            endWaitSequence();
            delay(3000);
            ESP.restart();
        } else {
            Serial.printf("Firmware update error %d\n", Update.getError());
        }
    } else {
        Update.abort();
    }

    return false;
}    

void unmount_fs()
{
    if(haveFS) {
        SPIFFS.end();
        #ifdef SID_DBG
        Serial.println(F("Unmounted Flash FS"));
        #endif
        haveFS = false;
    }
    if(haveSD) {
        SD.end();
        #ifdef SID_DBG
        Serial.println(F("Unmounted SD card"));
        #endif
        haveSD = false;
    }
}

/*
 * Various helpers
 */

void formatFlashFS()
{
    #ifdef SID_DBG
    Serial.println(F("Formatting flash FS"));
    #endif
    SPIFFS.format();
}



#if 0
/*
 * Helpers for JSON config files
 */
static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, const char *funcName)
{
    const char *buf = NULL;
    size_t bufSize = configFile.size();
    DeserializationError ret;

    if(!(buf = (const char *)malloc(bufSize + 1))) {
        Serial.printf("%s: Buffer allocation failed (%d)\n", funcName, bufSize);
        return DeserializationError::NoMemory;
    }

    memset((void *)buf, 0, bufSize + 1);

    configFile.read((uint8_t *)buf, bufSize);

    #ifdef SID_DBG
    Serial.println(buf);
    #endif
    
    ret = deserializeJson(json, buf);

    free((void *)buf);

    return ret;
}

static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, const char *funcName)
{
    char *buf;
    size_t bufSize = measureJson(json);
    bool success = false;

    if(!(buf = (char *)malloc(bufSize + 1))) {
        Serial.printf("%s: Buffer allocation failed (%d)\n", funcName, bufSize);
        return false;
    }

    memset(buf, 0, bufSize + 1);
    serializeJson(json, buf, bufSize);

    #ifdef SID_DBG
    Serial.printf("Writing %s to %s, %d bytes\n", fn, useSD ? "SD" : "FS", bufSize);
    Serial.println((const char *)buf);
    #endif

    if(useSD) {
        success = writeFileToSD(fn, (uint8_t *)buf, (int)bufSize);
    } else {
        success = writeFileToFS(fn, (uint8_t *)buf, (int)bufSize);
    }

    free(buf);

    if(!success) {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }

    return success;
}
#endif

#if 0
/*
 * Generic file readers/writes for external
 */
bool readFileFromSD(const char *fn, uint8_t *buf, int len)
{
    size_t bytesr;
    
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_READ);
    if(myFile) {
        bytesr = myFile.read(buf, len);
        myFile.close();
        return (bytesr == len);
    } else
        return false;
}

bool writeFileToSD(const char *fn, uint8_t *buf, int len)
{
    size_t bytesw;
    
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_WRITE);
    if(myFile) {
        bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}

bool readFileFromFS(const char *fn, uint8_t *buf, int len)
{
    size_t bytesr;
    
    if(!haveFS)
        return false;

    if(!SPIFFS.exists(fn))
        return false;

    File myFile = SPIFFS.open(fn, FILE_READ);
    if(myFile) {
        bytesr = myFile.read(buf, len);
        myFile.close();
        return (bytesr == len);
    } else
        return false;
}

bool writeFileToFS(const char *fn, uint8_t *buf, int len)
{
    size_t bytesw;
    
    if(!haveFS)
        return false;

    File myFile = SPIFFS.open(fn, FILE_WRITE);
    if(myFile) {
        bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}

bool openACFile(File& file)
{
    if(haveSD) {
        if(file = SD.open(CONFN, FILE_WRITE)) {
            return true;
        }
    }

    return false;
}

size_t writeACFile(File& file, uint8_t *buf, size_t len)
{
    return file.write(buf, len);
}

void closeACFile(File& file)
{
    file.close();
}

void removeACFile()
{
    if(haveSD) {
        SD.remove(CONFN);
    }
}
#endif
