#include "RotaryEncoder.hpp"
#include <wiringPi.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

RotaryEncoder::RotaryEncoder(int clkPin, int dtPin, int swPin)
    : _clkPin(clkPin), 
      _dtPin(dtPin), 
      _swPin(swPin),
      _position(0),
      _buttonPressed(false),
      _debounceMs(5),
      _longPressMs(500),
      _running(false) {
}

RotaryEncoder::~RotaryEncoder() {
    stop();
}

bool RotaryEncoder::begin() {
    // Initialize WiringPi
    if (wiringPiSetupGpio() == -1) {
        printf("Failed to initialize WiringPi\n");
        return false;
    }

    // Set up pins
    pinMode(_clkPin, INPUT);
    pinMode(_dtPin, INPUT);
    pinMode(_swPin, INPUT);
    
    pullUpDnControl(_clkPin, PUD_UP);
    pullUpDnControl(_dtPin, PUD_UP);
    pullUpDnControl(_swPin, PUD_UP);

    // Start monitoring thread
    _running = true;
    if (pthread_create(&_threadId, NULL, encoderThread, this) != 0) {
        printf("Failed to create encoder thread\n");
        return false;
    }

    return true;
}

void RotaryEncoder::stop() {
    if (_running) {
        _running = false;
        pthread_join(_threadId, NULL);
    }
}

void RotaryEncoder::onRotate(EncoderCallback callback) {
    _rotateCallback = callback;
}

void RotaryEncoder::onButtonPress(ButtonCallback callback) {
    _pressCallback = callback;
}

void RotaryEncoder::onButtonRelease(ButtonCallback callback) {
    _releaseCallback = callback;
}

void RotaryEncoder::onButtonLongPress(ButtonCallback callback) {
    _longPressCallback = callback;
}

void RotaryEncoder::setDebounceMs(uint32_t ms) {
    _debounceMs = ms;
}

void RotaryEncoder::setLongPressMs(uint32_t ms) {
    _longPressMs = ms;
}

void* RotaryEncoder::encoderThread(void* arg) {
    RotaryEncoder* encoder = static_cast<RotaryEncoder*>(arg);
    
    int lastClk = digitalRead(encoder->_clkPin);
    int lastBtn = digitalRead(encoder->_swPin);
    uint32_t btnPressTime = 0;
    bool longPressSent = false;

    while (encoder->_running) {
        // Handle rotation
        int clk = digitalRead(encoder->_clkPin);
        if (clk != lastClk) {
            delay(encoder->_debounceMs);  // Simple debounce
            int dt = digitalRead(encoder->_dtPin);
            
            if (clk != lastClk) {  // Make sure it wasn't a bounce
                if (dt != clk) {
                    encoder->_position++;
                    if (encoder->_rotateCallback) 
                        encoder->_rotateCallback(1);
                } else {
                    encoder->_position--;
                    if (encoder->_rotateCallback)
                        encoder->_rotateCallback(-1);
                }
            }
            lastClk = clk;
        }

        // Handle button
        int btn = digitalRead(encoder->_swPin);
        if (btn != lastBtn) {
            delay(encoder->_debounceMs);  // Simple debounce
            btn = digitalRead(encoder->_swPin);
            
            if (btn != lastBtn) {  // Make sure it wasn't a bounce
                if (btn == LOW) {  // Button pressed
                    encoder->_buttonPressed = true;
                    btnPressTime = millis();
                    longPressSent = false;
                    if (encoder->_pressCallback)
                        encoder->_pressCallback();
                } else {  // Button released
                    encoder->_buttonPressed = false;
                    if (encoder->_releaseCallback)
                        encoder->_releaseCallback();
                }
                lastBtn = btn;
            }
        }

        // Handle long press
        if (encoder->_buttonPressed && !longPressSent) {
            if (millis() - btnPressTime >= encoder->_longPressMs) {
                if (encoder->_longPressCallback)
                    encoder->_longPressCallback();
                longPressSent = true;
            }
        }

        delay(1);  // Don't hog the CPU
    }

    return NULL;
}
