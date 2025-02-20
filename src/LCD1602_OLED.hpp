#pragma once

#include <string>
#include "I2C.hpp"
#include "SSD1306.hpp"

// This class provides an LCD1602-compatible interface but drives an OLED display
class LCD1602 {
public:
    LCD1602(uint8_t i2cAddress = 0x3C);  // Default OLED address
    ~LCD1602();

    bool begin();
    void clear();
    void setCursor(uint8_t col, uint8_t row);
    void print(const std::string& text);
    void setBacklight(bool on);

private:
    SSD1306 _oled;
    uint8_t _curRow;
    uint8_t _curCol;
    bool _backlightOn;
};
