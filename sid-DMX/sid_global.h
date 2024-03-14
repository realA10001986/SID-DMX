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

#define SID_VERSION "V0.94"
#define SID_VERSION_EXTRA "MAR092024"

/*************************************************************************
 ***                        Build configuration                        ***
 *************************************************************************/

//#define SID_DBG              // debug output on Serial

// If this is uncommented, the firmware uses channel DMX_VERIFY_CHANNEL
// for packet verification. The value of this channel must, at all times,
// be DMX_VERIFY_VALUE for a packet to be accepted.
// Must be disabled (commented) if the DMX controller's blackout function 
// is to be used but lacks a way to exclude channels (like in case of 
// QLC+ version 4.x)
//#define DMX_USE_VERIFY

// Mode for "Effect ramp up" slider at DMX values 1 through 255:
// 0: slider goes through strict tt sequence (51 steps, stale)
// 1: slider works like GPS speed on original firmware 
//    (0-88mph; strict; including slight randomization up 75mph)
// 2: like 1, but non-strict
// 3: slider works like GPS speed on original firmware 
//    (30-88mph; strict; including slight randomization up 75mph)
// 4: like 3, but non-strict
//
// For 1 and 3: Levels are slightly randomized at 2Hz; if slider level 
// is beyond 75mph (=DMX value 215 in mode 1, and 200 in mode 3), 
// no more randomization is performed, since remaining steps are
// meant to resemble the authentic linear time travel sequence.
#define ERU_MODE 3


/*************************************************************************
 ***                             GPIO pins                             ***
 *************************************************************************/

// IR Remote (unused in DMX version)
#define IRREMOTE_PIN      27

// IR feedback
#define IR_FB_PIN         17

// Time Travel button (or TCD input trigger) (unused in DMX version)
#define TT_IN_PIN         13

// I2S audio pins (unused in DMX version)
#define I2S_BCLK_PIN      26
#define I2S_LRCLK_PIN     25
#define I2S_DIN_PIN       33

// SD Card pins
#define SD_CS_PIN          5
#define SPI_MOSI_PIN      23
#define SPI_MISO_PIN      19
#define SPI_SCK_PIN       18 

// DMX
#define DMX_TRANSMIT      14
#define DMX_RECEIVE       35
#define DMX_ENABLE        32

#endif
