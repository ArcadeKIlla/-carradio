#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string>
#include "EncoderBase.hpp"

class GenericEncoder : public EncoderBase {
public:
    GenericEncoder(int clkPin = -1, int dtPin = -1, int swPin = -1);
    virtual ~GenericEncoder();

    // EncoderBase interface implementation
    virtual bool begin() override;
    virtual void stop() override;
    virtual bool updateStatus() override;
    virtual bool wasClicked() override;
    virtual bool wasDoubleClicked() override;
    virtual bool wasMoved(bool &clockwise) override;
    virtual bool isPressed() override;
    virtual bool setAntiBounce(uint8_t period) override;
    virtual bool setDoubleClickTime(uint8_t period) override;

    // Additional methods
    bool begin(int clkPin, int dtPin, int swPin);

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
