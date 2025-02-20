#pragma once

#include <stdint.h>
#include <functional>

class RotaryEncoder {
public:
    // Callback types
    typedef std::function<void(int)> EncoderCallback;  // Called with direction (-1 or 1)
    typedef std::function<void(void)> ButtonCallback;   // Called when button pressed

    RotaryEncoder(int clkPin, int dtPin, int swPin);
    ~RotaryEncoder();

    bool begin();
    void stop();

    // Set callbacks
    void onRotate(EncoderCallback callback);
    void onButtonPress(ButtonCallback callback);
    void onButtonRelease(ButtonCallback callback);
    void onButtonLongPress(ButtonCallback callback);

    // Get current state
    int getPosition() const { return _position; }
    bool isButtonPressed() const { return _buttonPressed; }

    // Configure behavior
    void setDebounceMs(uint32_t ms);
    void setLongPressMs(uint32_t ms);

private:
    const int _clkPin;
    const int _dtPin;
    const int _swPin;
    
    volatile int _position;
    volatile bool _buttonPressed;
    uint32_t _debounceMs;
    uint32_t _longPressMs;
    
    EncoderCallback _rotateCallback;
    ButtonCallback _pressCallback;
    ButtonCallback _releaseCallback;
    ButtonCallback _longPressCallback;

    static void* encoderThread(void* arg);
    pthread_t _threadId;
    volatile bool _running;
};
