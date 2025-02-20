#pragma once

#include <string>
#include "I2C.hpp"
#include "SSD1306.hpp"

// This class provides an LCD-compatible interface for the SSD1306 OLED
class SSD1306_LCD {
public:
    SSD1306_LCD(uint8_t i2cAddress = 0x3C);  // Default OLED address
    ~SSD1306_LCD();

    bool begin();
    void clear();
    void setCursor(uint8_t col, uint8_t row);
    void print(const std::string& text);
    void setBacklight(bool on);  // For compatibility - OLEDs don't have backlight

private:
    SSD1306 _oled;
    uint8_t _curRow;
    uint8_t _curCol;
    bool _displayOn;
};
