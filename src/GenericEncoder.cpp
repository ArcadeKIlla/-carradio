#include "GenericEncoder.hpp"
#include <sys/time.h>
#include <gpiod.h>

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
    _chip = nullptr;
    _clkLine = nullptr;
    _dtLine = nullptr;
    _swLine = nullptr;
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
    _clkPin = clkPin;
    _dtPin = dtPin;
    _swPin = swPin;

    // Open GPIO chip
    _chip = gpiod_chip_open_by_name("gpiochip0");
    if (!_chip) {
        return false;
    }

    // Get GPIO lines
    _clkLine = gpiod_chip_get_line(_chip, _clkPin);
    _dtLine = gpiod_chip_get_line(_chip, _dtPin);
    _swLine = gpiod_chip_get_line(_chip, _swPin);

    if (!_clkLine || !_dtLine || !_swLine) {
        stop();
        return false;
    }

    // Configure lines as inputs with pull-up
    if (gpiod_line_request_input(_clkLine, "encoder-clk") < 0 ||
        gpiod_line_request_input(_dtLine, "encoder-dt") < 0 ||
        gpiod_line_request_input(_swLine, "encoder-sw") < 0) {
        stop();
        return false;
    }

    // Set initial states
    _lastClk = gpiod_line_get_value(_clkLine) == 1;
    _lastDt = gpiod_line_get_value(_dtLine) == 1;
    _lastSw = gpiod_line_get_value(_swLine) == 1;

    _isSetup = true;
    return true;
}

void GenericEncoder::stop() {
    if (_clkLine) {
        gpiod_line_release(_clkLine);
        _clkLine = nullptr;
    }
    if (_dtLine) {
        gpiod_line_release(_dtLine);
        _dtLine = nullptr;
    }
    if (_swLine) {
        gpiod_line_release(_swLine);
        _swLine = nullptr;
    }
    if (_chip) {
        gpiod_chip_close(_chip);
        _chip = nullptr;
    }
    _isSetup = false;
}

bool GenericEncoder::updateStatus() {
    if (!_isSetup) return false;

    _wasClicked = false;
    _wasDoubleClicked = false;
    _wasMoved = false;

    bool currentClk = gpiod_line_get_value(_clkLine) == 1;
    bool currentDt = gpiod_line_get_value(_dtLine) == 1;
    bool currentSw = gpiod_line_get_value(_swLine) == 1;

    // Check for rotation
    if (currentClk != _lastClk) {
        _wasMoved = true;
        _moveClockwise = currentDt != currentClk;
    }

    // Check for button press/release
    if (currentSw != _lastSw && !currentSw) {
        uint64_t now = getCurrentTimeMillis();
        
        if (_lastClickTime > 0 && 
            (now - _lastClickTime) <= _doubleClickPeriod) {
            _wasDoubleClicked = true;
            _lastClickTime = 0;
        } else {
            _wasClicked = true;
            _lastClickTime = now;
        }
        _lastPressTime = now;
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
    if (!_isSetup) return false;
    return gpiod_line_get_value(_swLine) == 0;
}

bool GenericEncoder::setAntiBounce(uint8_t period) {
    _antiBouncePeriod = period;
    return true;
}

bool GenericEncoder::setDoubleClickTime(uint8_t period) {
    _doubleClickPeriod = period;
    return true;
}
