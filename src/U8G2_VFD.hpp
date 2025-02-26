#pragma once

#include "VFD.hpp"
#include "u8g2.h"
#include "I2C.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>

using namespace std;

// U8G2 implementation of the VFD interface
class U8G2_VFD : public VFD {
public:
    U8G2_VFD(uint8_t i2cAddress = 0x3C) : _i2cAddress(i2cAddress), _isSetup(false) {
        // Initialize font pointers to nullptr
        _currentFont = nullptr;
        _font5x7 = nullptr;
        _font10x14 = nullptr;
        _fontMini = nullptr;
    }

    ~U8G2_VFD() {
        if (_isSetup) {
            // Clean up u8g2
            u8g2_SetPowerSave(&_u8g2, 1); // Display off
        }
    }

    bool begin(const char* devicePath = "/dev/i2c-1") override {
        if (_isSetup) return true;

        printf("U8G2_VFD: Initializing with device path %s\n", devicePath);

        // Initialize I2C
        if (!_i2c.begin(devicePath)) {
            printf("U8G2_VFD: Failed to initialize I2C on %s\n", devicePath);
            return false;
        }

        // Store device path for callbacks
        _devicePath = devicePath;

        // Initialize the u8g2 structure for SSD1306 128x64 OLED
        u8g2_Setup_ssd1306_i2c_128x64_noname_f(
            &_u8g2,
            U8G2_R0,
            u8x8_byte_hw_i2c,
            u8x8_gpio_and_delay_linux
        );

        // Set I2C address
        u8x8_SetI2CAddress(&_u8g2.u8x8, _i2cAddress << 1); // u8g2 expects shifted address

        // Initialize the display
        u8g2_InitDisplay(&_u8g2);
        u8g2_SetPowerSave(&_u8g2, 0); // Display on

        // Set up fonts
        _font5x7 = u8g2_font_5x7_tr;
        _font10x14 = u8g2_font_10x20_tr; // Closest match to 10x14
        _fontMini = u8g2_font_4x6_tr;    // Mini font

        // Set default font
        _currentFont = _font5x7;
        u8g2_SetFont(&_u8g2, _currentFont);

        // Clear the display
        clearScreen();

        _isSetup = true;
        printf("U8G2_VFD: Initialization complete\n");
        return true;
    }

    bool clearScreen() override {
        if (!_isSetup) return false;
        printf("U8G2_VFD: Clearing screen...\n");
        
        u8g2_ClearBuffer(&_u8g2);
        u8g2_SendBuffer(&_u8g2);
        
        // Reset cursor position
        _cursorX = 0;
        _cursorY = 0;
        
        return true;
    }

    bool write(string str) override {
        if (!_isSetup) return false;
        
        // In u8g2, y coordinate is the baseline of the text, not the top
        // Add font height to y to match VFD behavior
        int fontHeight = 0;
        if (_currentFont == _font5x7) fontHeight = 7;
        else if (_currentFont == _font10x14) fontHeight = 14;
        else if (_currentFont == _fontMini) fontHeight = 6;
        
        u8g2_DrawStr(&_u8g2, _cursorX, _cursorY + fontHeight, str.c_str());
        u8g2_SendBuffer(&_u8g2);
        
        // Update cursor position
        _cursorX += u8g2_GetStrWidth(&_u8g2, str.c_str());
        
        return true;
    }

    bool writePacket(const uint8_t* data, size_t len) override {
        if (!_isSetup) return false;
        
        // Process VFD command packet
        if (len > 0) {
            switch (data[0]) {
                case VFD_CLEAR_AREA:
                    if (len >= 5) {
                        // Clear a specific area
                        uint8_t x1 = data[1];
                        uint8_t y1 = data[2];
                        uint8_t x2 = data[3];
                        uint8_t y2 = data[4];
                        
                        printf("U8G2_VFD: Clearing area (%d,%d) to (%d,%d)\n", x1, y1, x2, y2);
                        
                        // Clear this area in the buffer
                        u8g2_SetDrawColor(&_u8g2, 0); // Black
                        u8g2_DrawBox(&_u8g2, x1, y1, x2-x1, y2-y1);
                        u8g2_SetDrawColor(&_u8g2, 1); // White
                        u8g2_SendBuffer(&_u8g2);
                    }
                    break;
                
                // Add other VFD commands as needed
                default:
                    // Unknown command, just write the data as raw bytes
                    for (size_t i = 0; i < len; i++) {
                        char c = static_cast<char>(data[i]);
                        write(string(1, c));
                    }
                    break;
            }
        }
        
        return true;
    }

    bool printPacket(const char* format, ...) override {
        if (!_isSetup) return false;
        
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        return write(string(buffer));
    }

    bool setFont(font_t font) override {
        if (!_isSetup) return false;
        
        switch (font) {
            case FONT_5x7:
                _currentFont = _font5x7;
                break;
            case FONT_10x14:
                _currentFont = _font10x14;
                break;
            case FONT_MINI:
                _currentFont = _fontMini;
                break;
            default:
                return false;
        }
        
        u8g2_SetFont(&_u8g2, _currentFont);
        return true;
    }

    bool setCursor(uint8_t x, uint8_t y) override {
        if (!_isSetup) return false;
        
        _cursorX = x;
        _cursorY = y;
        return true;
    }

    uint8_t width() override {
        return 128; // SSD1306 width
    }

    uint8_t height() override {
        return 64;  // SSD1306 height
    }

private:
    // GPIO and delay callback for u8g2
    static uint8_t u8x8_gpio_and_delay_linux(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
        switch(msg) {
            case U8X8_MSG_GPIO_AND_DELAY_INIT:
                // Initialize GPIO (not needed for I2C)
                return 1;
            case U8X8_MSG_DELAY_MILLI:
                // Delay in milliseconds
                usleep(arg_int * 1000);
                return 1;
            case U8X8_MSG_DELAY_10MICRO:
                // Delay in 10 microseconds
                usleep(arg_int * 10);
                return 1;
            case U8X8_MSG_DELAY_100NANO:
                // Delay in 100 nanoseconds (not possible with usleep, ignore)
                return 1;
            case U8X8_MSG_GPIO_DC:
            case U8X8_MSG_GPIO_RESET:
            case U8X8_MSG_GPIO_CS:
            case U8X8_MSG_GPIO_CLOCK:
            case U8X8_MSG_GPIO_DATA:
                // Not used for I2C
                return 1;
        }
        return 0;
    }

    u8g2_t _u8g2;                // u8g2 structure
    I2C _i2c;                    // I2C interface
    uint8_t _i2cAddress;         // I2C address
    bool _isSetup;               // Is the display initialized?
    uint8_t _cursorX, _cursorY;  // Cursor position
    const char* _devicePath;     // I2C device path
    
    // Font pointers
    const uint8_t* _currentFont;
    const uint8_t* _font5x7;
    const uint8_t* _font10x14;
    const uint8_t* _fontMini;
};
