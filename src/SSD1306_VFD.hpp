#pragma once

#include "VFD.hpp"
#include "SSD1306.hpp"
#include "ErrorMgr.hpp"
#include <string>
#include <stdarg.h>
#include <algorithm>

// Define OLED display constants
#define SSD1306_WHITE 1
#define SSD1306_BLACK 0

// Adapter class that makes SSD1306 OLED look like a VFD to DisplayMgr
class SSD1306_VFD : public VFD {
public:
    SSD1306_VFD(uint8_t i2cAddress = SSD1306::DEFAULT_I2C_ADDRESS) 
        : _oled(i2cAddress), _currentFont(FONT_MINI), _isSetup(false) {
        printf("SSD1306_VFD: Created with I2C address 0x%02X\n", i2cAddress);
    }
    
    ~SSD1306_VFD() override {
        stop();
    }

    bool begin(const char* path, speed_t speed) override {
        return init(path);
    }

    bool begin(const char* path, speed_t speed, int &error) override {
        if(!begin(path, speed)) {
            error = ENODEV;
            return false;
        }
        return true;
    }

    bool init(const char* path = "/dev/i2c-1") {
        printf("SSD1306_VFD: Initializing OLED display at %s\n", path);
        if (!_oled.begin(path)) {
            printf("SSD1306_VFD: Failed to initialize OLED display\n");
            return false;
        }
        _oled.clear();
        _oled.display();
        _isSetup = true;
        printf("SSD1306_VFD: OLED display initialized successfully\n");
        return true;
    }

    void stop() override {
        if (_isSetup) {
            _oled.clear();
            _oled.display();
            _isSetup = false;
        }
    }

    bool reset() override {
        _oled.clear();
        _oled.display();
        return true;
    }

    bool write(string str) override {
        if (!_isSetup) return false;
        _oled.print(str);
        _oled.display();
        return true;
    }

    bool write(const char* str) override {
        if (!_isSetup) return false;
        _oled.print(std::string(str));
        _oled.display();
        return true;
    }

    bool writePacket(const uint8_t *data, size_t len, useconds_t waitusec = 50) override {
        if (!_isSetup) return false;
        char* str = new char[len + 1];
        memcpy(str, data, len);
        str[len] = '\0';
        bool result = write(str);
        delete[] str;
        return result;
    }

    bool printPacket(const char *fmt, ...) override {
        if (!_isSetup) return false;
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        return write(buffer);
    }

    bool printLines(uint8_t y, uint8_t step, stringvector lines,
                   uint8_t firstLine, uint8_t maxLines,
                   VFD::font_t font = VFD::FONT_MINI) override {
        if (!_isSetup) return false;
        
        setFont(font);
        uint8_t lineHeight = (font == FONT_10x14) ? 16 : 8;
        
        for (uint8_t i = 0; i < maxLines && (firstLine + i) < lines.size(); i++) {
            _oled.setCursor(0, y + i * lineHeight);
            _oled.print(lines[firstLine + i]);
        }
        _oled.display();
        return true;
    }

    bool printRows(uint8_t y, uint8_t step, vector<vector<string>> columns,
                  uint8_t firstLine, uint8_t maxLines, uint8_t x_offset = 0,
                  VFD::font_t font = VFD::FONT_MINI) override {
        if (!_isSetup) return false;
        
        setFont(font);
        uint8_t lineHeight = (font == FONT_10x14) ? 16 : 8;
        uint8_t columnWidth = width() / (columns.empty() ? 1 : columns.size());
        
        for (uint8_t i = 0; i < maxLines && (firstLine + i) < columns[0].size(); i++) {
            for (uint8_t col = 0; col < columns.size(); col++) {
                _oled.setCursor(x_offset + col * columnWidth, y + i * lineHeight);
                _oled.print(columns[col][firstLine + i]);
            }
        }
        _oled.display();
        return true;
    }

    bool setBrightness(uint8_t level) override {
        if (!_isSetup) return false;
        // Map VFD brightness (0-7) to OLED contrast (0-255)
        uint8_t contrast = (level * 255) / 7;
        _oled.setContrast(contrast);
        return true;
    }

    bool setPowerOn(bool setOn) override {
        if (!_isSetup) return false;
        // OLED doesn't have direct power control, but we can clear the display
        if (!setOn) {
            _oled.clear();
            _oled.display();
        }
        return true;
    }

    bool clearScreen() override {
        if (!_isSetup) return false;
        _oled.clear();
        _oled.display();
        return true;
    }

    void drawScrollBar(uint8_t top, float bar_height, float starting_offset) override {
        if (!_isSetup) return;
        
        int16_t barX = width() - scroll_bar_width;
        int16_t barY = top;
        int16_t barH = static_cast<int16_t>(bar_height * height());
        int16_t offset = static_cast<int16_t>(starting_offset * height());
        
        // Draw scroll bar background
        _oled.drawRect(barX, 0, scroll_bar_width, height(), true, false);
        
        // Draw scroll bar handle
        _oled.drawRect(barX, barY + offset, scroll_bar_width, barH, true, true);
        _oled.display();
    }

    bool setCursor(uint8_t x, uint8_t y) override {
        if (!_isSetup) return false;
        _oled.setCursor(x, y);
        return true;
    }

    bool setFont(font_t font) override {
        if (!_isSetup) return false;
        _currentFont = font;
        switch (font) {
            case FONT_MINI:
                _oled.setTextSize(1);
                break;
            case FONT_5x7:
                _oled.setTextSize(1);
                break;
            case FONT_10x14:
                _oled.setTextSize(2);
                break;
        }
        return true;
    }

    uint16_t width() override {
        return SSD1306::DISPLAY_WIDTH;
    }

    uint16_t height() override {
        return SSD1306::DISPLAY_HEIGHT;
    }

private:
    SSD1306 _oled;
    font_t _currentFont;
    bool _isSetup;
};
