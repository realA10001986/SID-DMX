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

// Mode:
// 0: eru slider goes through strict tt sequence (50 steps)
// 1: eru is like GPS speed on original firmware (0-88; strict; including slight randomization)
// 2: eru is like GPS speed on original firmware (0-88; non-strict; including slight randomization)
// 3: eru is like GPS speed on original firmware (30-88; strict; including slight randomization)
// 4: eru is like GPS speed on original firmware (30-88; non-strict; including slight randomization)
int modeOfOperation = 3;

bool        strictMode = false;      // config
static bool useGPSS    = false;      // config

uint16_t    idleMode   = 0;

#define SID_IDLE_0    0
#define SID_IDLE_1    1
#define SID_IDLE_2    2
#define SID_IDLE_3    3

#define SBLF_REPEAT   1
#define SBLF_ISTT     2
#define SBLF_LM       4
#define SBLF_SKIPSHOW 8
#define SBLF_LMTT     16
#define SBLF_NOBL     32
#define SBLF_ANIM     64
#define SBLF_STRICT   128
static int            sidBaseLine = 0;
static int            strictBaseLine = 0;
static bool           blWayup = true;
static unsigned long  lastChange = 0;
static unsigned long  idleDelay = 800;
static uint8_t        oldIdleHeight[10] = { 
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19
};
static int  TTcnt = 0;
static int  TTClrBar = 0;
static int  TTClrBarInc = 1;
static int  TTBarCnt = 0;
static bool TTBri = false;
static bool usingGPSS = false;
static int  gpsSpeed = 0;
static int  prevGPSSpeed = -2;

int transmitPin = DMX_TRANSMIT;
int receivePin = DMX_RECEIVE;
int enablePin = DMX_ENABLE;

/* We have 3 ports (0-2). Port 0 is for the Serial Monitor. */
dmx_port_t dmxPort = 1;

dmx_packet_t packet;

uint8_t data[DMX_PACKET_SIZE];

#define DMX_ADDRESS   34
#define DMX_CHANNELS  12

#define DMX_VERIFY_CHANNEL 46    // must be set to DMX_VERIFY_VALUE
#define DMX_VERIFY_VALUE   100   

#if defined(DMX_USE_VERIFY) && (DMX_ADDRESS < DMX_VERIFY_CHANNEL)
#define DMX_SLOTS_TO_RECEIVE (DMX_VERIFY_CHANNEL + 1)
#else
#define DMX_SLOTS_TO_RECEIVE (DMX_ADDRESS + DMX_CHANNELS)
#endif

// DMX footprint
#define SID_BASE DMX_ADDRESS

unsigned long powerupMillis = 0;

uint8_t cache[DMX_CHANNELS];

static bool          dmxIsConnected = false;
static unsigned long lastDMXpacket = 0;

#define TT_SQF_LN 51
static const uint8_t ttledseqfull[TT_SQF_LN][10] = {
    {  1,  0,  0,  4,  0,  0,  0,  0,  0,  0 },
    {  2,  1,  0,  4,  0,  0,  0,  0,  0,  0 },
    {  3,  2,  0,  5,  0,  0,  0,  0,  0,  0 },
    {  4,  2,  0,  6,  0,  0,  0,  0,  0,  0 },
    {  4,  3,  0,  6,  0,  0,  0,  0,  0,  0 },
    {  4,  3,  0,  7,  0,  0,  0,  0,  0,  0 },
    {  4,  4,  0,  7,  0,  0,  0,  0,  0,  0 },
    {  4,  4,  0,  8,  0,  0,  0,  0,  0,  0 },
    {  4,  5,  0,  9,  0,  0,  0,  0,  0,  0 },
    {  4,  5,  0,  9,  0,  1,  0,  0,  0,  0 }, // 10
    {  5,  5,  0,  9,  0,  1,  0,  0,  0,  0 },
    {  5,  6,  0, 10,  0,  1,  0,  0,  0,  0 },
    {  5,  7,  0, 10,  0,  1,  0,  0,  0,  1 },
    {  5,  8,  0, 10,  0,  1,  0,  0,  0,  2 },
    {  6,  9,  0, 10,  0,  2,  0,  0,  0,  3 },
    {  6,  9,  0, 10,  0,  2,  1,  0,  0,  4 },
    {  6,  9,  0, 10,  0,  3,  1,  0,  0,  5 },
    {  6,  9,  0, 10,  0,  3,  2,  0,  0,  6 },
    {  6,  9,  0, 10,  0,  3,  3,  0,  0,  7 },
    {  6,  9,  0, 10,  0,  3,  4,  0,  0,  8 }, // 20
    {  6,  9,  0, 10,  0,  3,  4,  0,  0,  9 },
    {  7, 10,  0, 10,  0,  3,  4,  0,  0,  9 },
    {  7, 10,  0, 10,  0,  4,  5,  0,  1,  9 },
    {  8, 10,  0, 10,  0,  4,  6,  0,  1,  9 },
    {  8, 10,  0, 10,  0,  4,  7,  0,  2,  9 },
    {  8, 10,  0, 10,  0,  4,  8,  0,  2, 10 },
    {  8, 10,  0, 10,  0,  4,  8,  0,  3, 10 },
    {  8, 10,  0, 10,  0,  4,  9,  0,  3, 10 },
    {  8, 10,  0, 10,  0,  4, 10,  0,  3, 10 },
    {  9, 10,  0, 10,  0,  4, 10,  0,  4, 10 }, // 30
    {  9, 10,  0, 10,  0,  5, 10,  0,  5, 10 },
    {  9, 10,  0, 10,  0,  6, 10,  0,  6, 10 },
    {  9, 10,  0, 10,  0,  7, 10,  0,  7, 10 },
    { 10, 10,  0, 10,  0,  8, 10,  0,  8, 10 },
    { 10, 10,  0, 10,  0,  9, 10,  0,  9, 10 },
    { 10, 10,  1, 10,  0, 10, 10,  0, 10, 10 },
    { 10, 10,  1, 10,  0, 11, 10,  0, 11, 10 },
    { 10, 10,  2, 10,  0, 12, 10,  0, 12, 10 },
    { 11, 10,  2, 10,  0, 12, 10,  0, 13, 10 },
    { 12, 10,  3, 10,  0, 12, 10,  0, 14, 10 }, // 40
    { 13, 10,  3, 10,  0, 12, 10,  0, 15, 10 },
    { 14, 10,  3, 10,  0, 12, 10,  0, 16, 10 },
    { 15, 10,  4, 10,  0, 12, 10,  0, 17, 10 },
    { 16, 10,  4, 10,  0, 12, 10,  0, 18, 10 },
    { 17, 10,  4, 10,  0, 12, 10,  0, 19, 10 },
    { 18, 10,  6, 10,  0, 12, 10,  0, 20, 10 },
    { 19, 10,  6, 10,  0, 12, 10,  0, 20, 10 },
    { 19, 11,  6, 10,  0, 12, 11,  0, 20, 10 },   // 48
    { 20, 15,  7, 10,  0, 12, 15,  0, 20, 10 },   // 51
    { 20, 20, 10, 10,  5, 12, 20,  6, 20, 10 },   // 55-ish
    { 20, 20, 13, 20, 20, 19, 20, 10, 20, 17 }    // 60 - tt
};

#define TT_SQ_LN 29
static const uint8_t ttledseq[TT_SQ_LN][10] = {
//     1   2   3   4   5   6   7   8   9  10
    {  0,  1,  0,  0,  0,  0,  0,  0,  0,  0 },   // 0
    {  1,  2,  0,  2,  0,  0,  1,  0,  1,  1 },   // 1
    {  2,  3,  0,  2,  0,  1,  2,  0,  1,  2 },   // 2
    {  3,  4,  0,  3,  0,  1,  3,  0,  2,  3 },   // 3
    {  4,  5,  0,  3,  0,  1,  4,  0,  2,  4 },   // 4
    {  5,  6,  0,  5,  0,  2,  6,  0,  2,  5 },   // 5
    {  6,  7,  0,  7,  0,  2,  7,  0,  2,  7 },   // 6    bl 6
    {  7,  9,  0,  9,  0,  3,  8,  0,  3,  9 },   // 7    bl 8
    {  8, 10,  0, 10,  0,  4,  9,  0,  3, 10 },   // 8  m bl 10 (26)
    {  8, 10,  0, 10,  0,  5,  9,  0,  4, 10 },   // 9
    {  8, 10,  0, 10,  0,  5,  9,  0,  5, 10 },   // 10
    {  8, 10,  0, 10,  0,  6,  9,  0,  6, 10 },   // 11
    {  8, 10,  0, 10,  0,  8,  9,  0,  7, 10 },   // 12
    {  8, 10,  0, 10,  0, 10,  9,  0,  8, 10 },   // 13
    {  8, 10,  0, 10,  0, 10,  9,  0,  9, 10 },   // 14
    { 10, 10,  1, 10,  0, 10, 10,  0, 10, 10 },   // 15 m bl 10 (36)
    { 10, 10,  1, 10,  0, 10, 10,  0, 10, 10 },   // 16 m bl 10
    { 10, 10,  1, 10,  0, 11, 10,  0, 11, 10 },   // 17 m       (37)
    { 10, 10,  2, 10,  0, 11, 10,  0, 12, 10 },   // 18 
    { 11, 10,  3, 10,  0, 11, 10,  0, 13, 10 },   // 19 
    { 12, 10,  3, 10,  0, 12, 10,  0, 14, 10 },   // 20 
    { 13, 10,  3, 10,  0, 12, 10,  0, 15, 10 },   // 21 m       (41)
    { 14, 10,  4, 10,  0, 12, 10,  0, 16, 10 },   // 22 
    { 15, 10,  4, 10,  0, 12, 10,  0, 17, 10 },   // 23 
    { 16, 10,  4, 10,  0, 12, 10,  0, 18, 10 },   // 24 m       (44)
    { 19, 10,  6, 10,  0, 12, 10,  0, 20, 10 },   // 25 m       (47)
    { 20, 15,  7, 10,  0, 12, 15,  7, 20, 10 },   // 26 m       (n/a)
    { 20, 20, 10, 10,  5, 12, 20, 10, 20, 10 },   // 27 m       (n/a)
    { 20, 20, 13, 20, 20, 19, 20, 10, 20, 17 },   // 28 m       (60)
};
static const uint8_t seqEntry[21] = {
  //  0  1  2  3  4  5  6  7  8  9 10  11  12  13  14  15  16  17  18  19  20      baseline
      0, 1, 2, 3, 4, 5, 6, 6, 7, 7, 8, 15, 18, 19, 20, 21, 21, 22, 22, 23, 24   // index in sequ
};

#define STT_SQ_LN 52
static const uint8_t staleledseq[STT_SQ_LN][10] = {
//     1   2   3   4   5   6   7   8   9  10
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },   // 0
    {  1,  0,  0,  4,  0,  0,  0,  0,  0,  0 },
    {  2,  1,  0,  4,  0,  0,  0,  0,  0,  0 },
    {  3,  2,  0,  5,  0,  0,  0,  0,  0,  0 },
    {  4,  2,  0,  6,  0,  0,  0,  0,  0,  0 },
    {  4,  3,  0,  6,  0,  0,  0,  0,  0,  0 },
    {  4,  3,  0,  7,  0,  0,  0,  0,  0,  0 },
    {  4,  4,  0,  7,  0,  0,  0,  0,  0,  0 },
    {  4,  4,  0,  8,  0,  0,  0,  0,  0,  0 },
    {  4,  5,  0,  9,  0,  0,  0,  0,  0,  0 },
    {  4,  5,  0,  9,  0,  1,  0,  0,  0,  0 }, // 10
    {  5,  5,  0,  9,  0,  1,  0,  0,  0,  0 },
    {  5,  6,  0, 10,  0,  1,  0,  0,  0,  0 },
    {  5,  7,  0, 10,  0,  1,  0,  0,  0,  1 },
    {  5,  8,  0, 10,  0,  1,  0,  0,  0,  2 },
    {  6,  9,  0, 10,  0,  2,  0,  0,  0,  3 },
    {  6,  9,  0, 10,  0,  2,  1,  0,  0,  4 },
    {  6,  9,  0, 10,  0,  3,  1,  0,  0,  5 },
    {  6,  9,  0, 10,  0,  3,  2,  0,  0,  6 },
    {  6,  9,  0, 10,  0,  3,  3,  0,  0,  7 },
    {  6,  9,  0, 10,  0,  3,  4,  0,  0,  8 }, // 20
    {  6,  9,  0, 10,  0,  3,  4,  0,  0,  9 },
    {  7, 10,  0, 10,  0,  3,  4,  0,  0,  9 },
    {  7, 10,  0, 10,  0,  4,  5,  0,  1,  9 },
    {  8, 10,  0, 10,  0,  4,  6,  0,  1,  9 },
    {  8, 10,  0, 10,  0,  4,  7,  0,  2,  9 },
    {  8, 10,  0, 10,  0,  4,  8,  0,  2, 10 },
    {  8, 10,  0, 10,  0,  4,  8,  0,  3, 10 },
    {  8, 10,  0, 10,  0,  4,  9,  0,  3, 10 },
    {  8, 10,  0, 10,  0,  4, 10,  0,  3, 10 },
    {  9, 10,  0, 10,  0,  4, 10,  0,  4, 10 }, // 30
    {  9, 10,  0, 10,  0,  5, 10,  0,  5, 10 },
    {  9, 10,  0, 10,  0,  6, 10,  0,  6, 10 },
    {  9, 10,  0, 10,  0,  7, 10,  0,  7, 10 },
    { 10, 10,  0, 10,  0,  8, 10,  0,  8, 10 },
    { 10, 10,  0, 10,  0,  9, 10,  0,  9, 10 },
    { 10, 10,  1, 10,  0, 10, 10,  0, 10, 10 },
    { 10, 10,  1, 10,  0, 11, 10,  0, 11, 10 },
    { 10, 10,  2, 10,  0, 12, 10,  0, 12, 10 },
    { 11, 10,  2, 10,  0, 12, 10,  0, 13, 10 },
    { 12, 10,  3, 10,  0, 12, 10,  0, 14, 10 }, // 40
    { 13, 10,  3, 10,  0, 12, 10,  0, 15, 10 },
    { 14, 10,  3, 10,  0, 12, 10,  0, 16, 10 },
    { 15, 10,  4, 10,  0, 12, 10,  0, 17, 10 },
    { 16, 10,  4, 10,  0, 12, 10,  0, 18, 10 },
    { 17, 10,  4, 10,  0, 12, 10,  0, 19, 10 },
    { 18, 10,  6, 10,  0, 12, 10,  0, 20, 10 },
    { 19, 10,  6, 10,  0, 12, 10,  0, 20, 10 },
    { 19, 11,  6, 10,  0, 12, 11,  0, 20, 10 },   // 48
    { 20, 15,  7, 10,  0, 12, 15,  0, 20, 10 },   // 51
    { 20, 20, 10, 10,  5, 12, 20,  6, 20, 10 },   // 55-ish
    { 20, 20, 13, 20, 20, 19, 20, 10, 20, 17 }    // 60 - tt
};

static uint8_t efxRanges[256] = { 0 };

static bool setDisplay(int base);
static void showIdle(bool forceUpdate = false, bool freezeBaseLine = false);


/* Code start */

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

    float x = (float)(STT_SQ_LN - 1) + 0.9;
    for(int i = 0; i < 256; i++) {
        efxRanges[i] = x * (float)i / 255.0;
    }
    
    invalidateCache();

    switch(modeOfOperation) {
    case 0:
      useGPSS = false;
      break;
    case 1:
    case 3:
      useGPSS = true;
      strictMode = true;
      break;
    case 2:
    case 4:
      useGPSS = true;
      strictMode = false;
      break;
    }

    // Start the DMX stuff
    dmx_driver_install(dmxPort, &config, personalities, personality_count);
    dmx_set_pin(dmxPort, transmitPin, receivePin, enablePin);
}


/*********************************************************************************
 * 
 * loop
 *
 *********************************************************************************/

void dmx_loop() 
{
    bool forceUpdate = false;
                    
    if(dmx_receive_num(dmxPort, &packet, DMX_SLOTS_TO_RECEIVE, 0)) {
        
        lastDMXpacket = millis();
    
        if(!packet.err) {

            if(!dmxIsConnected) {
                Serial.println("DMX is connected");
                dmxIsConnected = true;
            }
      
            dmx_read(dmxPort, data, packet.size);
      
            if(!data[0]) {

                #ifdef DMX_USE_VERIFY
                if(data[DMX_VERIFY_CHANNEL] == DMX_VERIFY_VALUE) {
                #endif
                    
                    if(memcmp(cache, data + SID_BASE, DMX_CHANNELS)) {
                        forceUpdate = setDisplay(SID_BASE);
                        memcpy(cache, data + SID_BASE, DMX_CHANNELS);
                        #ifdef SID_DBG
                        Serial.println("setDisplay called");
                        #endif
                    }

                #ifdef DMX_USE_VERIFY
                } else {

                    Serial.printf("Bad verification value on channel %d: %d (should be %d)\n", 
                          DMX_VERIFY_CHANNEL, data[DMX_VERIFY_CHANNEL], DMX_VERIFY_VALUE);
                  
                }
                #endif
                
            } else {
              
                Serial.printf("Unrecognized start code %d (0x%02x)", data[0], data[0]);
                
            }
          
        } else {
            
            Serial.printf("DMX error: %d\n", packet.err);
            
        }

    }

    switch(modeOfOperation) {
    case 0:
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        if(gpsSpeed >= 0) {
            showIdle(forceUpdate);
        }
        break;
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
 * 0 = ch1:   Master brightness (0-255; 0=off; 1-255=darkest-brightest) 
 * 1 = ch2:   "Effect ramp up" (0-255); 0=off (use ch3-12); 1=idle ???; 2-255 ramp up to tt
 * 2 = ch3:   Col 1 (left-most) (0-255) |
 * 3 = ch4:   Col 2 (0-255)
 * 4 = ch5:   Col 3 (0-255)
 * 5 = ch6:   Col 4 (0-255)
 * 6 = ch7:   Col 5 (0-255)
 * 7 = ch8:   Col 6 (0-255)
 * 8 = ch9:   Col 7 (0-255)
 * 9 = ch10:  Col 8 (0-255)
 * 10 = ch11: Col 9  (0-255)
 * 11 = ch12: Col 10 (right-most) (0-255)
 * 
 */

static bool setDisplay(int base)
{ 
    bool forceupd = false;
    int  mbri = data[base + 0];
    int  eru = data[base + 1];
    
    if(mbri) {
        if(eru) {
            switch(modeOfOperation) {
            case 0:
                for(int i = 0; i < 10; i++) {
                    sid.drawBarWithHeight(i, staleledseq[efxRanges[eru]][i]);
                }
                sid.show();
                break;
            case 1:
            case 2:
                gpsSpeed = (int)((float)eru / 2.87);
                if(gpsSpeed > 75) forceupd = true;
                #ifdef SID_DBG
                Serial.printf("gpsSpeed %d\n", gpsSpeed);
                #endif
                break;
            case 3:
            case 4:
                gpsSpeed = (int)((float)eru / 4.329) + 30;
                if(gpsSpeed > 75) forceupd = true;
                #ifdef SID_DBG
                Serial.printf("gpsSpeed %d\n", gpsSpeed);
                #endif
                break;
            }
        } else {
            // manual pattern selection
            for(int i = 0; i < 10; i++) {
                sid.drawBarWithHeight(i, data[base + 2 + i] / 12);
            }
            sid.show();
            gpsSpeed = -1;
            prevGPSSpeed = -2;
        }
    }

    if(mbri) {    // master bri
        sid.on();
        sid.setBrightness(mbri / 16);
    } else {
        sid.off();
    }

    return forceupd;
}


static void showBaseLine(int variation, uint16_t flags)
{
    const int mods[21][10] = {
        { 130, 90, 10,  80,  10, 110, 100,  15, 120,  90 }, // g 0
        { 130, 90, 10,  80,  10, 110, 100,  15, 100,  90 }, // g 1
        { 130, 90, 20,  80,  15, 110, 100,  15, 120, 100 }, // g 2
        { 110,100, 70,  80,  30,  50, 100,  15, 100, 110 }, // g 3
        { 110,110, 40,  90,  30,  50, 100,  15,  80, 100 }, // g 4
        { 110,110, 30, 120,  30,  50, 110,  15,  50, 100 }, // g 5
        { 100,100, 20, 120,  10,  50, 110,  20,  40, 110 }, // g 6
        { 110,120, 15, 110,  20,  40, 110,  18,  40, 100 }, // g 7
        { 100,100, 15, 110,  20,  50, 100,  15,  50,  90 }, // g 8
        {  90,110,  0, 100,  20,  50, 100,  15,  60, 100 }, // g 9
        {  90,100, 10, 100,  10,  60,  90,  15,  40, 100 }, // g 10
        {  90,100, 10, 100,  10,  90,  90,  15, 110, 100 }, // g 11
        {  90, 90, 20,  90,  15, 100, 100,  50, 100,  90 }, // g 12
        {  90, 90, 20,  90,  15, 100, 100,  50, 100,  90 }, // y 13
        {  90, 80, 10,  80,  15,  90,  80,  50, 100,  80 }, // y 14
        {  90, 80, 10,  80,  15,  90,  80,  50, 100,  80 }, // y 15
        {  90, 80, 10,  80,  15,  90,  80,  50, 100,  80 }, // y 16
        {  90, 70, 20,  70,  15,  70,  70,  40, 100,  70 }, // y 17
        {  90, 70, 20,  70,  15,  70,  70,  40,  90,  70 }, // y 18
        {  90, 60, 25,  60,  15,  80,  60,  40,  90,  60 }, // r 19
        {  90, 90, 70, 100,  90, 110,  90,  60,  95,  80 }  // extra for TT
    };
    const uint8_t maxTTHeight[10] = {
        19, 19, 12, 19, 19, 18, 19,  9, 19, 16
    };

    int bh, a = sidBaseLine, b;
    int vc = (flags & SBLF_ISTT) ? 0 : variation / 2;

    if(!(flags & SBLF_NOBL)) {
          
        if(a < 0) a = 0;
        if(a > 19) a = 19;

        b = (flags & SBLF_ISTT) ? 20 : a;
    
        if(flags & SBLF_REPEAT) {
            // (Never set in strict mode)
            for(int i = 0; i < 10; i++) {
                sid.drawBar(i, 0, oldIdleHeight[i]);
            }
        } else {
            if(!(flags & SBLF_STRICT)) {
                for(int i = 0; i < 10; i++) {
                    bh = a * (mods[b][i] + ((int)(esp_random() % variation)-vc)) / 100;
                    if(bh < 0) bh = 0;
                    if(bh > 19) bh = 19;
                    //if((flags & SBLF_LM) && bh < 9) {
                    //    bh = 9 + (int)(esp_random() % 4);
                    //}
                    //if(!(flags & SBLF_ISTT) && abs(bh - oldIdleHeight[i]) > 5) {
                    //    bh = (oldIdleHeight[i] + bh) / 2;
                    //}
                    if(flags & SBLF_ISTT) {
                        if(bh > maxTTHeight[i] || (!(flags & SBLF_ANIM))) bh = maxTTHeight[i];
                    }
                    sid.drawBar(i, 0, bh);
                    oldIdleHeight[i] = bh;
                }
            } else {
                for(int i = 0; i < 10; i++) {
                    bh = ttledseqfull[strictBaseLine][i];
                    if(flags & SBLF_ISTT) {
                        if(bh > maxTTHeight[i] + 1 || (!(flags & SBLF_ANIM))) bh = maxTTHeight[i] + 1;
                    }
                    sid.drawBarWithHeight(i, bh);
                    if(bh > 0) bh--;
                    oldIdleHeight[i] = bh;
                }
            }
        }
    
        if(flags & SBLF_ISTT) {
            if(flags & SBLF_ANIM) {
                if(TTClrBarInc && !(flags & SBLF_LMTT)) {
                    sid.clearBar(TTClrBar);
                    TTClrBar += TTClrBarInc;
                    if(TTClrBar >= 10) {
                        TTClrBar = 9;
                        TTClrBarInc = -1;
                    }
                    if(TTClrBar < 0 && TTClrBarInc < 0) {
                        TTClrBarInc = 1;
                        TTClrBar = 0;
                        TTBarCnt++;
                        if(TTBarCnt > 0) TTBri = true;
                    }
                }
            }
            if(TTBri || (flags & SBLF_LMTT)) {
                int temp1 = sid.getBrightness(), temp2 = 3;
                if(temp1 >= 4) temp1 -= 2;
                else { temp1 = 2; temp2 = 0; }
                sid.setBrightnessDirect((esp_random() % temp1) + temp2);
            }
        } 

    }

    if(!(flags & SBLF_SKIPSHOW)) {
        sid.show();
    }

    #ifdef SID_DBG
    //Serial.printf("baseline %d, strict %d\n", sidBaseLine, strictBaseLine);
    #endif
}

static void showIdle(bool forceUpdate, bool freezeBaseLine)
{
    unsigned long now = millis();
    int oldBaseLine = sidBaseLine;
    int oldSBaseLine = strictBaseLine;
    int variation = 20;
    uint16_t sblFlags = 0;

    if(useGPSS && gpsSpeed >= 0) {

        if(!forceUpdate && (now - lastChange < 500))
            return;

        usingGPSS = true;
        
        lastChange = now;

        if(!gpsSpeed) {

            if(gpsSpeed != prevGPSSpeed) {
                sid.clearDisplayDirect();
                prevGPSSpeed = gpsSpeed;
            }
            return;

        } else if(!strictMode) {
            if(!freezeBaseLine) {
                if(gpsSpeed >= 88) {
                    sidBaseLine = 19;
                    sblFlags |= SBLF_ISTT;
                } else {
                    sidBaseLine = (max(10, (int)gpsSpeed) * 20 / 88) - 1;
                    if(sidBaseLine > 19) sidBaseLine = 19;
                    variation = 10;
                }
                //if(abs(oldBaseLine - sidBaseLine) > 3) {
                //    sidBaseLine = (sidBaseLine + oldBaseLine) / 2;
                //}
            }
        } else {
            if(!freezeBaseLine) {
                strictBaseLine = gpsSpeed * 100 / (88 * 100 / (TT_SQF_LN - 1));
                if(gpsSpeed == prevGPSSpeed) {
                    if(strictBaseLine < 5) {
                        strictBaseLine += (esp_random() % 5);
                    } else if(strictBaseLine > TT_SQF_LN - 9) {
                        // no modify at > 75mph
                    } else {
                        strictBaseLine += (((esp_random() % 5)) - 2);
                    }
                    if(strictBaseLine < 0) strictBaseLine = 0;
                    //if(strictBaseLine > TT_SQF_LN-2) strictBaseLine = TT_SQF_LN-2;
                }
                if(strictBaseLine > TT_SQF_LN-1) strictBaseLine = TT_SQF_LN-1;
                //if(abs(oldSBaseLine - strictBaseLine) > 3) {
                //    strictBaseLine = (strictBaseLine + oldSBaseLine) / 2;
                //}
            }
            sblFlags |= SBLF_STRICT;
        }

        prevGPSSpeed = gpsSpeed;

        //Serial.printf("spd %d  bl %d %d\n", gpsSpeed, strictBaseLine, sidBaseLine);

    } else {
        
        if(!forceUpdate && (now - lastChange < idleDelay))
            return;
          
        lastChange = now;

        if(strictMode) {
            sblFlags |= SBLF_STRICT;
        }

        switch(idleMode) {
        case SID_IDLE_1:     // higher peaks, tempo as 0
            idleDelay = 800 + ((int)(esp_random() % 200) - 100);
            if(!strictMode) {
                if(!freezeBaseLine) {
                    if(sidBaseLine > 16) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine > 12) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine < 3) {
                        sidBaseLine += (((esp_random() % 3)) + 2);
                    } else {
                        sidBaseLine += (((esp_random() % 5)) - 1);
                    }
                    variation = 40;
                }
            } else {
                if(!freezeBaseLine) {
                    if(strictBaseLine > 40) {
                        strictBaseLine -= (((esp_random() % 5)) + 1);
                        blWayup = false;
                    } else if(strictBaseLine < 10) {
                        strictBaseLine += (((esp_random() % 5)) + 2);
                        blWayup = true;
                    } else {
                        strictBaseLine += (((esp_random() % 7)) - (blWayup ? 2 : 4));
                    }
                } else {
                    if((esp_random() % 5) >= 2) {
                        strictBaseLine ^= 0x01;   // toggle bit 0, nothing more
                    }
                }
            }
            break;
        case SID_IDLE_2:       // Same as 0, but faster
            idleDelay = 300 + ((int)(esp_random() % 200) - 100);
            if(!strictMode) {
                if(!freezeBaseLine) {
                    if(sidBaseLine > 14) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine > 8) {
                        sidBaseLine -= (((esp_random() % 5)) + 1);
                    } else if(sidBaseLine < 3) {
                        sidBaseLine += (((esp_random() % 3)) + 2);
                    } else {
                        sidBaseLine += (((esp_random() % 4)) - 1);
                    }
                }
            } else {
                if(!freezeBaseLine) {
                    if(strictBaseLine > 30) {
                        strictBaseLine -= (((esp_random() % 3)) + 1);
                        blWayup = false;
                    } else if(strictBaseLine < 10) {
                        strictBaseLine += (((esp_random() % 3)) + 2);
                        blWayup = true;
                    } else {
                        strictBaseLine += (((esp_random() % 7)) - (blWayup ? 2 : 4));
                    }
                } else {
                    if((esp_random() % 5) >= 2) {
                        strictBaseLine ^= 0x01;   // toggle bit 0, nothing more
                    }
                }
            }
            break;
        case SID_IDLE_3:     // higher peaks, faster
            idleDelay = 300 + ((int)(esp_random() % 200) - 100);
            if(!strictMode) {
                if(!freezeBaseLine) {
                    if(sidBaseLine > 16) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine > 12) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine < 3) {
                        sidBaseLine += (((esp_random() % 3)) + 2);
                    } else {
                        sidBaseLine += (((esp_random() % 5)) - 1);
                    }
                    variation = 40;
                }
            } else {
                if(!freezeBaseLine) {
                    if(strictBaseLine > 40) {
                        strictBaseLine -= (((esp_random() % 5)) + 1);
                        blWayup = false;
                    } else if(strictBaseLine < 10) {
                        strictBaseLine += (((esp_random() % 5)) + 2);
                        blWayup = true;
                    } else {
                        strictBaseLine += (((esp_random() % 7)) - (blWayup ? 2 : 4));
                    }
                } else {
                    if((esp_random() % 5) >= 2) {
                        strictBaseLine ^= 0x01;   // toggle bit 0, nothing more
                    }
                }
            }
            break;
        default:
            idleDelay = 800 + ((int)(esp_random() % 200) - 100);
            if(!strictMode) {
                if(!freezeBaseLine) {
                    if(sidBaseLine > 14) {
                        sidBaseLine -= (((esp_random() % 3)) + 1);
                    } else if(sidBaseLine > 8) {
                        sidBaseLine -= (((esp_random() % 5)) + 1);
                    } else if(sidBaseLine < 3) {
                        sidBaseLine += (((esp_random() % 3)) + 2);
                    } else {
                        sidBaseLine += (((esp_random() % 4)) - 1);
                    }
                }
            } else {
                if(!freezeBaseLine) {
                    if(strictBaseLine > 30) {
                        strictBaseLine -= (((esp_random() % 3)) + 1);
                        blWayup = false;
                    } else if(strictBaseLine < 10) {
                        strictBaseLine += (((esp_random() % 3)) + 2);
                        blWayup = true;
                    } else {
                        strictBaseLine += (((esp_random() % 7)) - (blWayup ? 2 : 4));
                    }
                } else {
                    if((esp_random() % 5) >= 2) {
                        strictBaseLine ^= 0x01;   // toggle bit 0, nothing more
                    }
                }
            }
            break;
        }
        
        if(!freezeBaseLine) {
            if(usingGPSS) {
                // Smoothen
                if(!(sblFlags & SBLF_STRICT)) {
                    if(abs(oldBaseLine - sidBaseLine) > 3) {
                        sidBaseLine = (sidBaseLine + oldBaseLine) / 2;
                    }
                } else {
                    if(abs(oldSBaseLine - strictBaseLine) > 7) {
                        strictBaseLine = (strictBaseLine + oldSBaseLine) / 2;
                    }
                }
                usingGPSS = false;
            }
        }
    }

    if(sidBaseLine < 0) sidBaseLine = 0;
    else if(sidBaseLine > 19) sidBaseLine = 19;
    if(strictBaseLine < 0) strictBaseLine = 0;
    else if(strictBaseLine > TT_SQF_LN-1) strictBaseLine = TT_SQF_LN-1;
    
    showBaseLine(variation, sblFlags);

    if(sblFlags & SBLF_SKIPSHOW) {
        sid.show();
    }
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
