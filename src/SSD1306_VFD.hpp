#pragma once

#include "VFD.hpp"
#include "SSD1306.hpp"
#include "ErrorMgr.hpp"

// Adapter class that makes SSD1306 OLED look like a VFD to DisplayMgr
class SSD1306_VFD : public VFD {
public:
    SSD1306_VFD(uint8_t i2cAddress = SSD1306::DEFAULT_I2C_ADDRESS) 
        : _oled(i2cAddress) {}
    
    ~SSD1306_VFD() {
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
        _oled.clear();
        _oled.display();
    }

    bool reset() override {
        _oled.clear();
        _oled.display();
        return true;
    }

    bool clear() override {
        _oled.clear();
        _oled.display();
        return true;
    }

    bool setLine(int line, const string& str) override {
        // Convert VFD line position to OLED coordinates
        // Assuming 8 pixel high characters and 2 lines
        _oled.setCursor(0, line * 8);
        _oled.print(str);
        _oled.display();
        return true;
    }

    bool setBrightness(uint8_t level) override {
        // Map VFD brightness (0-3) to OLED contrast (0-255)
        uint8_t contrast = (level * 255) / 3;
        _oled.setContrast(contrast);
        return true;
    }

private:
    SSD1306 _oled;
};
