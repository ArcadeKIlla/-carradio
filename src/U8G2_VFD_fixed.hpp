#pragma once

#include "VFD.hpp"
#include "u8g2.h"
#include "I2C.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>

using namespace std;

// Forward declaration of hardware I2C function
uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

// U8G2 implementation of the VFD interface
class U8G2_VFD : public VFD {
public:
    U8G2_VFD(uint8_t i2cAddress = 0x3C) : _i2cAddress(i2cAddress), _isSetup(false) {
        // Initialize font pointers to nullptr
        _currentFont = nullptr;
        _font5x7 = nullptr;
        _font10x14 = nullptr;
        _fontMini = nullptr;
        _fd = -1;
    }

    ~U8G2_VFD() {
        if (_isSetup) {
            // Clean up u8g2
            u8g2_SetPowerSave(&_u8g2, 1); // Display off
        }
    }

    // Implement the VFD interface methods with matching signatures
    bool begin(const char* path, speed_t speed = B19200) override {
        int error = 0;
        return begin(path, speed, error);
    }

    bool begin(const char* path, speed_t speed, int &error) override {
        if (_isSetup) return true;

        printf("U8G2_VFD: Initializing with device path %s\n", path);

        // Initialize I2C with the device address
        if (!_i2c.begin(_i2cAddress, path, error)) {
            printf("U8G2_VFD: Failed to initialize I2C on %s (error: %d)\n", path, error);
            return false;
        }

        // Store device path for callbacks
        _devicePath = path;

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

    void stop() override {
        if (_isSetup) {
            u8g2_SetPowerSave(&_u8g2, 1); // Display off
            _i2c.stop();
            _isSetup = false;
        }
    }

    bool reset() override {
        if (!_isSetup) return false;
        
        // Reset the display
        u8g2_SetPowerSave(&_u8g2, 1); // Display off
        usleep(100000); // 100ms delay
        u8g2_SetPowerSave(&_u8g2, 0); // Display on
        clearScreen();
        
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

    bool write(const char* str) override {
        return write(string(str));
    }

    bool writePacket(const uint8_t *data, size_t len, useconds_t waitusec = 50) override {
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
                
                case VFD_SET_CURSOR:
                    if (len >= 3) {
                        uint8_t x = data[1];
                        uint8_t y = data[2];
                        setCursor(x, y);
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
        
        // Honor the wait time
        if (waitusec > 0) {
            usleep(waitusec);
        }
        
        return true;
    }

    bool printPacket(const char *fmt, ...) override {
        if (!_isSetup) return false;
        
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        
        return write(string(buffer));
    }

    bool printLines(uint8_t y, uint8_t step, stringvector lines,
                    uint8_t firstLine, uint8_t maxLines,
                    VFD::font_t font = VFD::FONT_MINI) override {
        if (!_isSetup) return false;
        
        setFont(font);
        
        uint8_t count = 0;
        for (size_t i = firstLine; i < lines.size() && count < maxLines; i++, count++) {
            setCursor(0, y + count * step);
            write(lines[i]);
        }
        
        return true;
    }

    bool printRows(uint8_t y, uint8_t step, vector<vector<string>> columns,
                   uint8_t firstLine, uint8_t maxLines, uint8_t x_offset = 0,
                   VFD::font_t font = VFD::FONT_MINI) override {
        if (!_isSetup) return false;
        
        setFont(font);
        
        uint8_t count = 0;
        for (size_t i = firstLine; i < columns.size() && count < maxLines; i++, count++) {
            uint8_t x = x_offset;
            for (size_t j = 0; j < columns[i].size(); j++) {
                setCursor(x, y + count * step);
                write(columns[i][j]);
                x += u8g2_GetStrWidth(&_u8g2, columns[i][j].c_str()) + 5; // Add spacing
            }
        }
        
        return true;
    }

    bool setBrightness(uint8_t brightness) override {
        if (!_isSetup) return false;
        
        // U8G2 doesn't have direct brightness control for SSD1306
        // We could implement it through contrast control
        uint8_t contrast = (brightness * 255) / 7; // Scale 0-7 to 0-255
        u8g2_SetContrast(&_u8g2, contrast);
        
        return true;
    }

    bool setPowerOn(bool setOn) override {
        if (!_isSetup) return false;
        
        u8g2_SetPowerSave(&_u8g2, setOn ? 0 : 1);
        return true;
    }

    bool setCursor(uint8_t x, uint8_t y) override {
        if (!_isSetup) return false;
        
        _cursorX = x;
        _cursorY = y;
        return true;
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

    void drawScrollBar(uint8_t top, float bar_height, float starting_offset) override {
        if (!_isSetup) return;
        
        uint8_t display_height = height();
        uint8_t display_width = width();
        
        // Draw the scroll bar outline
        u8g2_DrawFrame(&_u8g2, display_width - scroll_bar_width, top, 
                      scroll_bar_width, display_height - top);
        
        // Calculate the scroll bar position and size
        uint8_t bar_y = top + (starting_offset * (display_height - top));
        uint8_t bar_h = bar_height * (display_height - top);
        
        // Draw the scroll bar
        u8g2_DrawBox(&_u8g2, display_width - scroll_bar_width + 1, bar_y, 
                    scroll_bar_width - 2, bar_h);
        
        u8g2_SendBuffer(&_u8g2);
    }

    uint16_t width() override {
        return 128; // SSD1306 width
    }

    uint16_t height() override {
        return 64;  // SSD1306 height
    }

    bool isSetup() const override {
        return _isSetup;
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
    int _fd;                     // File descriptor (required by VFD base class)
    
    // Font pointers
    const uint8_t* _currentFont;
    const uint8_t* _font5x7;
    const uint8_t* _font10x14;
    const uint8_t* _fontMini;
};
