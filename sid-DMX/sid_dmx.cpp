/*
 * -------------------------------------------------------------------
 * CircuitSetup.us SID - DMX-controlled
 * (C) 2024 Thomas Winischhofer (A10001986)
 * All rights reserved.
 * -------------------------------------------------------------------
 */

#include "sid_global.h"

#include <Arduino.h>
#include <esp_dmx.h>

#include "sid_dmx.h"
#include "siddisplay.h"

// The SID display object
sidDisplay sid(0x74, 0x72);

int transmitPin = DMX_TRANSMIT;
int receivePin = DMX_RECEIVE;
int enablePin = DMX_ENABLE;

/* We have 3 ports (0-2). Port 0 is for the Serial Monitor. */
dmx_port_t dmxPort = 1;

dmx_packet_t packet;

uint8_t data[DMX_PACKET_SIZE];

#define DMX_ADDRESS   34
#define DMX_CHANNELS  2   //10

#define DMX_SLOTS_TO_RECEIVE (DMX_ADDRESS + DMX_CHANNELS)

// DMX footprint
#define SID_BASE DMX_ADDRESS

static bool fullScale = false;

unsigned long powerupMillis;

uint8_t cache[DMX_CHANNELS];

static bool          dmxIsConnected = false;
static unsigned long lastDMXpacket;

#define TT_SQ_LN 30
static const uint8_t ttledseq[TT_SQ_LN][10] = {
//     1   2   3   4   5   6   7   8   9  10
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },   // 0
    {  0,  1,  0,  0,  0,  0,  0,  0,  0,  0 },   // 1
    {  1,  2,  0,  2,  0,  0,  1,  0,  1,  1 },   // 2
    {  2,  3,  0,  2,  0,  1,  2,  0,  1,  2 },   // 3
    {  3,  4,  0,  3,  0,  1,  3,  0,  2,  3 },   // 4
    {  4,  5,  0,  3,  0,  1,  4,  0,  2,  4 },   // 5
    {  5,  6,  0,  5,  0,  2,  6,  0,  2,  5 },   // 6
    {  6,  7,  0,  7,  0,  2,  7,  0,  2,  7 },   // 7    bl 6
    {  7,  9,  0,  9,  0,  3,  8,  0,  3,  9 },   // 8    bl 8
    {  8, 10,  0, 10,  0,  4,  9,  0,  3, 10 },   // 9  m bl 10 (26)
    {  8, 10,  0, 10,  0,  5,  9,  0,  4, 10 },   // 10
    {  8, 10,  0, 10,  0,  5,  9,  0,  5, 10 },   // 11
    {  8, 10,  0, 10,  0,  6,  9,  0,  6, 10 },   // 12
    {  8, 10,  0, 10,  0,  8,  9,  0,  7, 10 },   // 13
    {  8, 10,  0, 10,  0, 10,  9,  0,  8, 10 },   // 14
    {  8, 10,  0, 10,  0, 10,  9,  0,  9, 10 },   // 15
    { 10, 10,  1, 10,  0, 10, 10,  0, 10, 10 },   // 16 m bl 10 (36)
    { 10, 10,  1, 10,  0, 10, 10,  0, 10, 10 },   // 17 m bl 10
    { 10, 10,  1, 10,  0, 11, 10,  0, 11, 10 },   // 18 m       (37)
    { 10, 10,  2, 10,  0, 11, 10,  0, 12, 10 },   // 19 
    { 11, 10,  3, 10,  0, 11, 10,  0, 13, 10 },   // 20 
    { 12, 10,  3, 10,  0, 12, 10,  0, 14, 10 },   // 21 
    { 13, 10,  3, 10,  0, 12, 10,  0, 15, 10 },   // 22 m       (41)
    { 14, 10,  4, 10,  0, 12, 10,  0, 16, 10 },   // 23 
    { 15, 10,  4, 10,  0, 12, 10,  0, 17, 10 },   // 24 
    { 16, 10,  4, 10,  0, 12, 10,  0, 18, 10 },   // 25 m       (44)
    { 19, 10,  6, 10,  0, 12, 10,  0, 20, 10 },   // 26 m       (47)
    { 20, 15,  7, 10,  0, 12, 15,  7, 20, 10 },   // 27 m       (n/a)
    { 20, 20, 10, 10,  5, 12, 20, 10, 20, 10 },   // 28 m       (n/a)
    { 20, 20, 13, 20, 20, 19, 20, 10, 20, 17 },   // 29 m       (60)
};

static uint8_t efxRanges[256] = { 0 };

static void setDisplay(int base);

static void invalidateCache()
{
    for(int i = 0; i < DMX_CHANNELS; i++) {
        cache[i] = rand() % 255;
    }
}

/*********************************************************************************
 * 
 * boot
 *
 *********************************************************************************/

void dmx_boot() 
{
    // Boot display, keep it dark
    sid.begin();

    // Init and turn off IR feedback LED
    pinMode(IR_FB_PIN, OUTPUT);
    digitalWrite(IR_FB_PIN, LOW);
}    



/*********************************************************************************
 * 
 * setup
 *
 *********************************************************************************/


void dmx_setup()
{
    dmx_config_t config = {
      .interrupt_flags = (DMX_INTR_FLAGS_DEFAULT | ESP_INTR_FLAG_LEVEL3 | ESP_INTR_FLAG_LEVEL2),
      .root_device_parameter_count = 32,
      .sub_device_parameter_count = 0,
      .model_id = 0,
      .product_category = RDM_PRODUCT_CATEGORY_FIXTURE,
      .software_version_id = 1,
      .software_version_label = "SID-DMXv1",
      .queue_size_max = 32
    };
    dmx_personality_t personalities[] = {
        {DMX_CHANNELS, "SID Personality"}
    };
    int personality_count = 1;

    Serial.println(F("SID DMX version " SID_VERSION " " SID_VERSION_EXTRA));
    Serial.println(F("(C) 2024 Thomas Winischhofer (A10001986)"));

    float x = (float)(TT_SQ_LN - 1) + 0.9;
    for(int i = 0; i < 256; i++) {
        efxRanges[i] = x * (float)i / 255.0;
    }
    
    invalidateCache();

    // Start the DMX stuff
    dmx_driver_install(dmxPort, &config, personalities, personality_count);
    dmx_set_pin(dmxPort, transmitPin, receivePin, enablePin);
}


/*********************************************************************************
 * 
 * loop
 *
 *********************************************************************************/

int zeroCnt = 0;

void dmx_loop() 
{
    bool isAllZero = true;
    bool doUpdate = true;
                    
    //size_t dmxRec = dmx_receive_num(dmxPort, &packet, DMX_SLOTS_TO_RECEIVE, 0) // DMX_TIMEOUT_TICK);
    
    if(dmx_receive_num(dmxPort, &packet, DMX_SLOTS_TO_RECEIVE, 0)) {
        
        lastDMXpacket = millis();
    
        if(!packet.err) {

            if(!dmxIsConnected) {
                Serial.println("DMX is connected");
                dmxIsConnected = true;
            }
      
            dmx_read(dmxPort, data, packet.size);
      
            if(!data[0]) {

                for(int i = SID_BASE; i < SID_BASE+DMX_CHANNELS; i++) {
                   if(data[i]) {
                        isAllZero = false;
                        break;
                   }
                }
                if(isAllZero) {
                    #ifdef SID_DBG 
                    Serial.printf("Zero packet: %d (%d)\n", packet.size, zeroCnt);
                    #endif
                    if(zeroCnt < 2) {
                        zeroCnt++;
                        doUpdate = false;
                    } else {
                        zeroCnt = 0;
                    }
                } else {
                    zeroCnt = 0;
                }
                if(doUpdate) {
                    if(memcmp(cache, data + SID_BASE, DMX_CHANNELS)) {
                        setDisplay(SID_BASE);
                        memcpy(cache, data + SID_BASE, DMX_CHANNELS);
                    }
                }
                
            } else {
              
                Serial.printf("Unrecognized start code %d (0x%02x)", data[0], data[0]);
                
            }
          
        } else {
            
            Serial.printf("DMX error: %d\n", packet.err);
            
        }

    }

    if(dmxIsConnected && (millis() - lastDMXpacket > 1250)) {
        Serial.println("DMX was disconnected");
        dmxIsConnected = false;
        invalidateCache();
    }
}



/*********************************************************************************
 * 
 * helpers
 *
 *********************************************************************************/


/*
 * 0 = ch1 Master brightness (0-255) 
 * 1 = ch2 "Effect ramp up" (0-255)
 * 
 * 
 Old mapping:
  0 = ch1:  Col 1 (left-most) (0-255) |
  1 = ch2:  Col 2                     |
  2 = ch3:  Col 3                     | Disregarded if ch 11 
  3 = ch4:  Col 4                     | is non-zero
  4 = ch5:  Col 5                     | 
  5 = ch6:  Col 6                     |
  6 = ch7:  Col 7                     |
  7 = ch8:  Col 8                     |
  8 = ch9:  Col 9                     |
  9 = ch10: Col 10                    |
 10 = ch11: Movie pattern idx (0-255)
 11 = ch12: Master brightness (0-255) 
*/

static void setDisplay(int base)
{
    int mbri = data[base + 0] / 15;
    int eru = data[base + 1];

    #ifdef SID_DBG
    Serial.printf("%x %x\n", data[base + 0], data[base + 1]);
    #endif

    if(mbri > 16) mbri = 16;
    
    if(mbri) {
        for(int i = 0; i < 10; i++) {
            sid.drawBarWithHeight(i, ttledseq[efxRanges[eru]][i]);
        }
        sid.show();
    }

    if(mbri) {    // master bri
        sid.on();
        sid.setBrightness(mbri - 1);
    } else {
        sid.off();
    }


    /*
    if(fullScale) {
        
        for(int i = base + 0; i < 10; i++) {
            data[i] /= 12;
            // range check done in sid class
        }

        data[base + 10] /= TT_SQ_LN;
        if(data[base + 10] >= TT_SQ_LN) data[base + 10] = TT_SQ_LN;

        data[base + 11] /= 15; // Brightness
        if(data[base + 11] == 255) data[base + 11]--;
        
    }

    mbri = data[base + 11];
    
    if(data[base + 9]) {
        // Movie pattern
        if(mbri) {    // master bri
            for(int i = 0; i < 10; i++) {
                sid.drawBarWithHeight(i, ttledseq[data[base + 11]][i]);
            }
        }
    } else {
        // manual pattern selection
        for(int i = 0; i < 10; i++) {
            sid.drawBarWithHeight(i, data[base + i]);
        }
        sid.show();
    }

    if(mbri) {    // master bri
        sid.on();
        sid.setBrightness(mbri - 1);
    } else {
        sid.off();
    }
    */
}

void showWaitSequence()
{
    // Show a "wait" symbol
    sid.clearDisplayDirect();
    sid.drawLetterAndShow('&', 0, 8);
}

void endWaitSequence()
{
    sid.clearDisplayDirect();
}

void showCopyError()
{
    for(int i = 0; i < 10; i++) {
        digitalWrite(IR_FB_PIN, HIGH);
        delay(250);
        digitalWrite(IR_FB_PIN, LOW);
    }
}
