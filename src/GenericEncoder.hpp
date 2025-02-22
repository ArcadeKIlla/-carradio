#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string>

class GenericEncoder {
public:
    GenericEncoder();
    ~GenericEncoder();

    // Initialize with GPIO pin numbers
    bool begin(int clkPin, int dtPin, int swPin);
    void stop();

    // Status checks
    bool updateStatus();
    bool wasClicked();
    bool wasDoubleClicked();
    bool wasMoved(bool &clockwise);
    bool isPressed();

    // Configuration
    bool setAntiBounce(uint8_t period);
    bool setDoubleClickTime(uint8_t period);

private:
    bool _isSetup;
    int _clkPin;
    int _dtPin;
    int _swPin;
    
    // State tracking
    bool _lastClk;
    bool _lastDt;
    bool _lastSw;
    uint64_t _lastPressTime;
    uint64_t _lastClickTime;
    uint8_t _antiBouncePeriod;
    uint8_t _doubleClickPeriod;
    
    bool _wasClicked;
    bool _wasDoubleClicked;
    bool _wasMoved;
    bool _moveClockwise;
};
