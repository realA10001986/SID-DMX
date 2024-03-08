/*
 * -------------------------------------------------------------------
 * CircuitSetup.us SID - DMX-controlled
 * (C) 2024 Thomas Winischhofer (A10001986)
 * All rights reserved.
 * -------------------------------------------------------------------
 */

#ifndef _SID_GLOBAL_H
#define _SID_GLOBAL_H

/*************************************************************************
 ***                          Version Strings                          ***
 *************************************************************************/

#define SID_VERSION "V0.93"
#define SID_VERSION_EXTRA "MAR082024"

#define SID_DBG              // debug output on Serial

// If this is uncommented, the firmware uses channel DMX_VERIFY_CHANNEL
// for packet verification. The value of this channel must, at all times,
// be DMX_VERIFY_VALUE for a packet to be accepted.
#define DMX_USE_VERIFY


/*************************************************************************
 ***                             GPIO pins                             ***
 *************************************************************************/

// IR Remote
#define IRREMOTE_PIN      27

// IR feedback
#define IR_FB_PIN         17

// Time Travel button (or TCD input trigger)
#define TT_IN_PIN         13

// I2S audio pins
#define I2S_BCLK_PIN      26
#define I2S_LRCLK_PIN     25
#define I2S_DIN_PIN       33

// SD Card pins
#define SD_CS_PIN          5
#define SPI_MOSI_PIN      23
#define SPI_MISO_PIN      19
#define SPI_SCK_PIN       18 

// DMX
#define DMX_TRANSMIT 14
#define DMX_RECEIVE  35 //13
#define DMX_ENABLE   32

#endif
