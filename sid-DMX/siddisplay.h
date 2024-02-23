/*
 * -------------------------------------------------------------------
 * CircuitSetup.us SID - DMX-controlled
 * (C) 2024 Thomas Winischhofer (A10001986)
 * All rights reserved.
 * -------------------------------------------------------------------
 */

#ifndef _SIDDISPLAY_H
#define _SIDDISPLAY_H

#define SD_BUF_SIZE   16  // Buffer size in words (16bit)

class sidDisplay {

    public:

        sidDisplay(uint8_t address1, uint8_t address2);
        void begin();
        void on();
        void off();

        void lampTest();

        void clearBuf();

        uint8_t setBrightness(uint8_t level, bool setInitial = false);
        void    resetBrightness();
        uint8_t setBrightnessDirect(uint8_t level);
        uint8_t getBrightness();
        
        void show();

        void clearDisplayDirect();

        void drawBar(uint8_t bar, uint8_t bottom, uint8_t top);
        void drawBarWithHeight(uint8_t bar, uint8_t height);
        void clearBar(uint8_t bar);
        void drawDot(uint8_t bar, uint8_t dot_y);

        void drawFieldAndShow(uint8_t *fieldData);

        void drawLetterAndShow(char alpha, int x = 0, int y = 8);
        void drawLetterMask(char alpha, int x, int y);
        void drawClockAndShow(uint8_t *dateBuf, int dx, int dy);

    private:
        void directCmd(uint8_t val);
        
        uint8_t _address[2] = { 0, 0 };

        uint8_t _brightness = 15;     // current display brightness
        uint8_t _origBrightness = 15; // value from settings
        
        uint16_t _displayBuffer[SD_BUF_SIZE];

};

#endif
