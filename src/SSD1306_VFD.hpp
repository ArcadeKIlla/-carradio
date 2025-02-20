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
        : _oled(i2cAddress), _currentFont(FONT_MINI) {}
    
    ~SSD1306_VFD() override {
        stop();
    }

    bool begin(const char* path, speed_t speed) override {
        // I2C setup is handled in SSD1306::begin()
        return _oled.begin();
    }

    bool begin(const char* path, speed_t speed, int &error) override {
        if(!begin(path, speed)) {
            error = ENODEV;
            return false;
        }
        return true;
    }

    void stop() override {
        _oled.clearDisplay();
        _oled.display();
    }

    bool reset() override {
        _oled.clearDisplay();
        _oled.display();
        return true;
    }

    bool clear() {
        _oled.clearDisplay();
        _oled.display();
        return true;
    }

    bool setLine(int line, const string& str) {
        // Convert VFD line position to OLED coordinates
        // Assuming 8 pixel high characters and 2 lines
        _oled.setCursor(0, line * 8);
        write(str);
        return true;
    }

    bool setBrightness(uint8_t level) override {
        // Map VFD brightness (0-3) to OLED contrast (0-255)
        uint8_t contrast = (level * 255) / 3;
        _oled.setContrast(contrast);
        return true;
    }

    bool write(string str) override {
        _oled.print(str);
        _oled.display();
        return true;
    }

    bool write(const char* str) override {
        _oled.print(str);
        _oled.display();
        return true;
    }

    bool writePacket(const uint8_t *data, size_t len, useconds_t waitusec = 50) override {
        // For OLED, we don't need the wait time as it's not a serial device
        // Just write the data as a string
        char* str = new char[len + 1];
        memcpy(str, data, len);
        str[len] = '\0';
        bool result = write(str);
        delete[] str;
        return result;
    }

    bool clearScreen() override {
        return clear(); // Reuse existing clear() implementation
    }

    bool setCursor(uint8_t x, uint8_t y) override {
        _oled.setCursor(x, y);
        return true;
    }

    bool printPacket(const char *fmt, ...) override {
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        return write(buffer);
    }

    bool setFont(font_t font) override {
        _currentFont = font;
        switch(font) {
            case FONT_MINI:
                _oled.setTextSize(1); // 6x8 pixels per character
                break;
            case FONT_5x7:
                _oled.setTextSize(1); // Similar to MINI for OLED
                break;
            case FONT_10x14:
                _oled.setTextSize(2); // 12x16 pixels per character
                break;
            default:
                _oled.setTextSize(1);
                break;
        }
        return true;
    }

    bool printLines(uint8_t y, uint8_t step, stringvector lines,
                   uint8_t firstLine, uint8_t maxLines,
                   VFD::font_t font = VFD::FONT_MINI) override {
        if (lines.empty()) return true;

        setFont(font);
        uint8_t lineHeight = getCurrentFontHeight();
        
        // Calculate actual number of lines to display
        size_t availableLines = lines.size() - firstLine;
        uint8_t numLines = static_cast<uint8_t>(std::min(static_cast<size_t>(maxLines), availableLines));
        
        for (uint8_t i = 0; i < numLines; i++) {
            // Use step if provided, otherwise use font height
            uint8_t yPos = y + (step > 0 ? i * step : i * lineHeight);
            setCursor(0, yPos);
            write(lines[firstLine + i]);
        }
        
        _oled.display();
        return true;
    }

    bool printRows(uint8_t y, uint8_t step, vector<vector<string>> columns,
                  uint8_t firstLine, uint8_t maxLines, uint8_t x_offset = 0,
                  VFD::font_t font = VFD::FONT_MINI) override {
        if (columns.empty()) return true;

        setFont(font);
        uint8_t charWidth = getCurrentFontWidth();
        uint8_t lineHeight = getCurrentFontHeight();
        
        // Calculate actual number of lines to display
        size_t availableLines = columns[0].size() - firstLine;
        uint8_t numLines = static_cast<uint8_t>(std::min(static_cast<size_t>(maxLines), availableLines));
        
        for (uint8_t line = 0; line < numLines; line++) {
            uint8_t x = x_offset;
            
            // Use step if provided, otherwise use font height
            uint8_t yPos = y + (step > 0 ? line * step : line * lineHeight);
            
            // Print each column in this row
            for (size_t col = 0; col < columns.size(); col++) {
                if (line + firstLine < columns[col].size()) {
                    setCursor(x, yPos);
                    write(columns[col][line + firstLine]);
                    
                    // Move x position for next column based on font width
                    x += columns[col][line + firstLine].length() * charWidth + charWidth/2;
                }
            }
        }
        
        _oled.display();
        return true;
    }

    virtual void drawScrollBar(uint8_t x_start, float bar_height, float offset) override {
        const uint8_t SCROLL_BAR_WIDTH = 4;
        uint8_t y_start = static_cast<uint8_t>(offset * height());
        uint8_t actual_height = static_cast<uint8_t>(bar_height * height());

        // Draw outline (not filled)
        _oled.drawRect(x_start, 0, SCROLL_BAR_WIDTH, DISPLAY_HEIGHT, false, SSD1306_WHITE);

        // Draw filled portion
        _oled.drawRect(x_start, y_start, SCROLL_BAR_WIDTH, actual_height, false, SSD1306_WHITE);
        _oled.display();
    }

    uint16_t width() override {
        return 128; // SSD1306 is 128 pixels wide
    }
    
    uint16_t height() override {
        return 64;  // SSD1306 is 64 pixels high
    }

private:
    SSD1306 _oled;
    font_t _currentFont;

    // Helper methods for font dimensions
    uint8_t getCurrentFontWidth() const {
        switch(_currentFont) {
            case FONT_MINI:
            case FONT_5x7:
                return 6;  // 6 pixels wide at size 1
            case FONT_10x14:
                return 12; // 12 pixels wide at size 2
            default:
                return 6;
        }
    }

    uint8_t getCurrentFontHeight() const {
        switch(_currentFont) {
            case FONT_MINI:
            case FONT_5x7:
                return 8;  // 8 pixels high at size 1
            case FONT_10x14:
                return 16; // 16 pixels high at size 2
            default:
                return 8;
        }
    }
};
