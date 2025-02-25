#pragma once

#include "SSD1306.hpp"

// SH1106-specific commands
#define SH1106_SETLOWCOLUMN 0x00
#define SH1106_SETHIGHCOLUMN 0x10
#define SH1106_SETPAGE 0xB0

class SH1106 : public SSD1306 {
public:
    SH1106(uint8_t i2cAddress = DEFAULT_I2C_ADDRESS) : SSD1306(i2cAddress) {}
    
    // Override display method to use SH1106 addressing
    void display() override;
    
private:
    // Column offset (SH1106 has 132 columns, we use 128 with 2 pixel offset)
    static const uint8_t COLUMN_OFFSET = 2;
};
