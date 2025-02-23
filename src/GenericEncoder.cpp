#include "GenericEncoder.hpp"
#include <wiringPi.h>
#include <sys/time.h>

static uint64_t getCurrentTimeMillis() {
    struct timeval te; 
    gettimeofday(&te, nullptr);
    return te.tv_sec * 1000LL + te.tv_usec / 1000;
}

GenericEncoder::GenericEncoder(int clkPin, int dtPin, int swPin) {
    _isSetup = false;
    _antiBouncePeriod = 1;
    _doubleClickPeriod = 50;
    _wasClicked = false;
    _wasDoubleClicked = false;
    _wasMoved = false;
    _clkPin = clkPin;
    _dtPin = dtPin;
    _swPin = swPin;
}

GenericEncoder::~GenericEncoder() {
    stop();
}

bool GenericEncoder::begin() {
    if (_clkPin < 0 || _dtPin < 0 || _swPin < 0) {
        return false;
    }
    return begin(_clkPin, _dtPin, _swPin);
}

bool GenericEncoder::begin(int clkPin, int dtPin, int swPin) {
    if (wiringPiSetupGpio() == -1) {
        return false;
    }

    _clkPin = clkPin;
    _dtPin = dtPin;
    _swPin = swPin;

    pinMode(_clkPin, INPUT);
    pinMode(_dtPin, INPUT);
    pinMode(_swPin, INPUT);
    
    pullUpDnControl(_clkPin, PUD_UP);
    pullUpDnControl(_dtPin, PUD_UP);
    pullUpDnControl(_swPin, PUD_UP);

    _lastClk = digitalRead(_clkPin);
    _lastDt = digitalRead(_dtPin);
    _lastSw = digitalRead(_swPin);
    _lastPressTime = 0;
    _lastClickTime = 0;

    _isSetup = true;
    return true;
}

void GenericEncoder::stop() {
    if (_isSetup) {
        // Reset pins to default state
        pinMode(_clkPin, INPUT);
        pinMode(_dtPin, INPUT);
        pinMode(_swPin, INPUT);
        _isSetup = false;
    }
}

bool GenericEncoder::updateStatus() {
    if (!_isSetup) return false;

    uint64_t currentTime = getCurrentTimeMillis();
    
    // Reset state flags
    _wasClicked = false;
    _wasDoubleClicked = false;
    _wasMoved = false;

    // Read current states
    bool currentClk = digitalRead(_clkPin);
    bool currentDt = digitalRead(_dtPin);
    bool currentSw = digitalRead(_swPin);

    // Handle rotation
    if (currentClk != _lastClk && currentTime - _lastPressTime > _antiBouncePeriod) {
        _wasMoved = true;
        _moveClockwise = (currentDt != currentClk);
        _lastPressTime = currentTime;
    }

    // Handle button press
    if (currentSw != _lastSw && currentTime - _lastPressTime > _antiBouncePeriod) {
        if (currentSw == LOW) {  // Button pressed
            if (currentTime - _lastClickTime < _doubleClickPeriod) {
                _wasDoubleClicked = true;
                _lastClickTime = 0;  // Reset to prevent triple clicks
            } else {
                _wasClicked = true;
                _lastClickTime = currentTime;
            }
            _lastPressTime = currentTime;
        }
    }

    _lastClk = currentClk;
    _lastDt = currentDt;
    _lastSw = currentSw;

    return true;
}

bool GenericEncoder::wasClicked() {
    return _wasClicked;
}

bool GenericEncoder::wasDoubleClicked() {
    return _wasDoubleClicked;
}

bool GenericEncoder::wasMoved(bool &clockwise) {
    if (_wasMoved) {
        clockwise = _moveClockwise;
        return true;
    }
    return false;
}

bool GenericEncoder::isPressed() {
    return _isSetup && digitalRead(_swPin) == LOW;
}

bool GenericEncoder::setAntiBounce(uint8_t period) {
    _antiBouncePeriod = period;
    return true;
}

bool GenericEncoder::setDoubleClickTime(uint8_t period) {
    _doubleClickPeriod = period;
    return true;
}
