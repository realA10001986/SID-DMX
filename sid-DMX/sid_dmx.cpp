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

/* Next, lets decide which DMX port to use. The ESP32 has either 2 or 3 ports.
  Port 0 is typically used to transmit serial data back to your Serial Monitor,
  so we shouldn't use that port. Lets use port 1! */
dmx_port_t dmxPort = 1;

dmx_packet_t packet;

uint8_t data[DMX_PACKET_SIZE];

// DMX footprint
#define SID_BASE 34

static bool fullScale = false;

unsigned long powerupMillis;

static bool          dmxIsConnected = false;
static unsigned long lastDMXpacket;

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

static void setDisplay(int base);


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
      .interrupt_flags = DMX_INTR_FLAGS_DEFAULT,
      .root_device_parameter_count = 32,
      .sub_device_parameter_count = 0,
      .model_id = 0,
      .product_category = RDM_PRODUCT_CATEGORY_FIXTURE,
      .software_version_id = 1,
      .software_version_label = "FC-DMXv1",
      .queue_size_max = 32
    };
    dmx_personality_t personalities[] = {
        {10, "FC Personality"}
    };
    int personality_count = 1;

    Serial.println(F("SID DMX version " SID_VERSION " " SID_VERSION_EXTRA));
    
  
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
    
    if(dmx_receive(dmxPort, &packet, 0)) {
        
        lastDMXpacket = millis();
    
        if (!packet.err) {

            if(!dmxIsConnected) {
                Serial.println("DMX is connected!");
                dmxIsConnected = true;
            }
      
            dmx_read(dmxPort, data, packet.size);
      
            if(!data[0]) {
                setDisplay(SID_BASE);
            } else {
                Serial.printf("Unrecognized start code %d (0x%02x)", data[0]);
            }
          
        } else {
            
            Serial.println("A DMX error occurred.");
            
        }
        
    } 

    if(dmxIsConnected && (millis() - lastDMXpacket > 1250)) {
        Serial.println("DMX was disconnected.");
        dmxIsConnected = false;
    }
}



/*********************************************************************************
 * 
 * helpers
 *
 *********************************************************************************/


/*
  0 = ch1:  Col 1 (left-most) (0-20)  |
  1 = ch2:  Col 2                     |
  2 = ch3:  Col 3                     | Disregarded if ch 11 
  3 = ch4:  Col 4                     | is non-zero
  4 = ch5:  Col 5                     | (0-20) (fs: 0-255)
  5 = ch6:  Col 6                     |
  6 = ch7:  Col 7                     |
  7 = ch8:  Col 8                     |
  8 = ch9:  Col 9                     |
  9 = ch10: Col 10                    |
 10 = ch11: Movie pattern idx (0; 1-29) (fs: 0-255)
 11 = ch12: Master brightness (0-16)    (fs: 0-255) 
           
*/

static void setDisplay(int base)
{
    int mbri;

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
}

void showWaitSequence(/*bool force*/)
{
    // Show a "wait" symbol
    //if(force) sid.on();
    sid.clearDisplayDirect();
    sid.drawLetterAndShow('&', 0, 8);
}

void endWaitSequence()
{
    sid.clearDisplayDirect();
}


void showCopyError()
{
    // TODO
}
