//
//  VFD_SH1106_Adapter.hpp
//  carradio_v2_sh1106
//
//  Created on 2/26/2025
//  This adapter converts VFD commands to SH1106 OLED display commands using the u8g2 library
//  Optimized for Raspberry Pi deployment
//

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <cmath>      // For ceil function

// Use relative path for u8g2 library - this allows more flexibility in installation
#include "U8g2lib.h"

#ifdef __linux__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <termios.h>  // For speed_t and B19200
#endif

using namespace std;

// This class implements the same interface as the original VFD class
// but uses the u8g2 library to drive an SH1106 OLED display
class VFD_SH1106_Adapter {
    
public:
    // Constants from original VFD class
    static constexpr uint8_t VFD_OUTLINE = 0x14;
    static constexpr uint8_t VFD_CLEAR_AREA = 0x12;
    static constexpr uint8_t VFD_SET_AREA = 0x11;
    static constexpr uint8_t VFD_SET_CURSOR = 0x10;
    static constexpr uint8_t VFD_SET_WRITEMODE = 0x1A;

    static constexpr uint8_t scroll_bar_width = 3;

    typedef enum {
        FONT_MINI = 0,
        FONT_5x7,
        FONT_10x14,
    } font_t;

    VFD_SH1106_Adapter();
    ~VFD_SH1106_Adapter();
    
    // Initialize with I2C - default address for most SH1106 displays is 0x3C
    bool begin(const char* path, speed_t speed = B19200);
    bool begin(const char* path, speed_t speed, int &error);
    bool begin(const char* path, uint8_t i2cAddress = 0x3C); // Raspberry Pi specific
    void stop();

    bool reset();
 
    bool write(string str);
    bool write(const char* str);
    bool writePacket(const uint8_t *data, size_t len, useconds_t waitusec = 50);

    bool printPacket(const char *fmt, ...);
    
    bool printLines(uint8_t y, uint8_t step, vector<string> lines,
                     uint8_t firstLine, uint8_t maxLines,
                     font_t font = FONT_MINI);

    bool printRows(uint8_t y, uint8_t step, vector<vector<string>> columns,
                    uint8_t firstLine, uint8_t maxLines, uint8_t x_offset = 0,
                    font_t font = FONT_MINI);

    bool setBrightness(uint8_t);  //  0 == off - 7 == max
    bool setPowerOn(bool setOn);
    
    bool clearScreen();
    
    void drawScrollBar(uint8_t top, float bar_height, float starting_offset);
 
    bool setCursor(uint8_t x, uint8_t y);
    bool setFont(font_t font);

    inline uint16_t width() { return 128; };
    inline uint16_t height() { return 64; };

private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C* _u8g2;
    bool _isSetup;
    uint8_t _cursorX;
    uint8_t _cursorY;
    font_t _currentFont;
    uint8_t _i2cAddress;
    
    // Helper functions
    void updateFont();
    void sendBuffer();
};

// Define a type alias to maintain compatibility with the original code
typedef vector<string> stringvector;

// Helper function for string truncation (from the original VFD implementation)
inline string truncate(const string& str, size_t width) {
    if (str.length() > width)
        return str.substr(0, width);
    return str;
}
