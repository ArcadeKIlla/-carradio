#pragma once

#include "I2C.hpp"
#include <string>
#include <vector>

class SSD1306 {
public:
    static const uint8_t DISPLAY_WIDTH = 128;
    static const uint8_t DISPLAY_HEIGHT = 64;
    static const uint8_t DEFAULT_I2C_ADDRESS = 0x3C;

    SSD1306(uint8_t i2cAddress = DEFAULT_I2C_ADDRESS);
    ~SSD1306();

    bool begin(const char* path = "/dev/i2c-1");
    void clear();
    void display();
    void setContrast(uint8_t contrast);
    void invertDisplay(bool invert);
    
    // Text functions
    void setCursor(uint8_t x, uint8_t y);
    void setTextSize(uint8_t size);
    void print(const std::string& text);
    
    // Graphics functions
    void drawPixel(int16_t x, int16_t y, bool white);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool white);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool fill, bool white);

private:
    bool sendCommand(uint8_t command);
    void sendData(uint8_t data);
    void sendData(const std::vector<uint8_t>& buffer);
    
    I2C _i2c;
    uint8_t _i2cAddress;
    std::vector<uint8_t> _buffer;
    uint8_t _cursorX;
    uint8_t _cursorY;
    uint8_t _textSize;
};
