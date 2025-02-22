#pragma once

#include <stdint.h>
#include <stdbool.h>

class EncoderBase {
public:
    virtual ~EncoderBase() {}

    virtual bool begin() = 0;
    virtual void stop() = 0;
    virtual bool updateStatus() = 0;
    virtual bool wasClicked() = 0;
    virtual bool wasDoubleClicked() = 0;
    virtual bool wasMoved(bool &clockwise) = 0;
    virtual bool isPressed() = 0;
    virtual bool setAntiBounce(uint8_t period) = 0;
    virtual bool setDoubleClickTime(uint8_t period) = 0;
};
