#include "RotaryEncoder.hpp"
#include <pigpio.h>
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
    // Initialize pigpio
    if (gpioInitialise() < 0) {
        printf("Failed to initialize pigpio\n");
        return false;
    }

    // Set up pins
    gpioSetMode(_clkPin, PI_INPUT);
    gpioSetMode(_dtPin, PI_INPUT);
    gpioSetMode(_swPin, PI_INPUT);
    
    gpioSetPullUpDown(_clkPin, PI_PUD_UP);
    gpioSetPullUpDown(_dtPin, PI_PUD_UP);
    gpioSetPullUpDown(_swPin, PI_PUD_UP);

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
        gpioTerminate();
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
    
    int lastClk = gpioRead(encoder->_clkPin);
    int lastBtn = gpioRead(encoder->_swPin);
    uint32_t btnPressTime = 0;
    bool longPressSent = false;

    while (encoder->_running) {
        // Handle rotation
        int clk = gpioRead(encoder->_clkPin);
        if (clk != lastClk) {
            delay(encoder->_debounceMs);  // Simple debounce
            int dt = gpioRead(encoder->_dtPin);
            
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
        int btn = gpioRead(encoder->_swPin);
        if (btn != lastBtn) {
            delay(encoder->_debounceMs);  // Simple debounce
            btn = gpioRead(encoder->_swPin);
            
            if (btn != lastBtn) {  // Make sure it wasn't a bounce
                if (btn == LOW) {  // Button pressed
                    encoder->_buttonPressed = true;
                    btnPressTime = time(NULL);
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
            if (time(NULL) - btnPressTime >= encoder->_longPressMs / 1000) {
                if (encoder->_longPressCallback)
                    encoder->_longPressCallback();
                longPressSent = true;
            }
        }

        delay(1);  // Don't hog the CPU
    }

    return NULL;
}
