#include "DisplayMgr.hpp"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Duppa I2CEncoderV2 knobs
constexpr uint8_t leftKnobAddress = 0x40;
constexpr uint8_t rightKnobAddress = 0x41;
constexpr uint8_t displayAddress = 0x3C;  // OLED display address

DisplayMgr::DisplayMgr() 
    : _rightKnob(rightKnobAddress),
      _leftKnob(leftKnobAddress),
      _display(displayAddress),
      _mode(MODE_UNKNOWN),
      _shouldAutoPlay(false) {
}

DisplayMgr::~DisplayMgr() {
    stop();
}

bool DisplayMgr::begin(const char* path, speed_t speed) {
    int error;
    return begin(path, speed, error);
}

bool DisplayMgr::begin(const char* path, speed_t speed, int& error) {
    // Initialize display
    if (!_display.begin()) {
        printf("Failed to initialize display\n");
        return false;
    }

    // Initialize knobs
    if (!_rightKnob.begin() || !_leftKnob.begin()) {
        printf("Failed to initialize knobs\n");
        return false;
    }

    _mode = MODE_STARTUP;
    showStartup();
    return true;
}

void DisplayMgr::stop() {
    _mode = MODE_UNKNOWN;
    clearDisplay();
}

bool DisplayMgr::reset() {
    clearDisplay();
    _mode = MODE_STARTUP;
    showStartup();
    return true;
}

void DisplayMgr::clearDisplay() {
    _display.clear();
}

void DisplayMgr::updateDisplay() {
    // Called after making changes to ensure display is updated
    _display.display();
}

bool DisplayMgr::setBrightness(double level) {
    // OLED doesn't support brightness directly
    // Could implement using contrast
    return true;
}

bool DisplayMgr::setKnobBackLight(bool isOn) {
    // Implement knob backlight control
    return true;
}

bool DisplayMgr::setKnobColor(knob_id_t knob, RGB color) {
    // Implement knob LED color control
    return true;
}

void DisplayMgr::showTime() {
    clearDisplay();
    _display.setCursor(0, 0);
    _display.print("12:00");  // Replace with actual time
    updateDisplay();
}

void DisplayMgr::showMessage(string message, time_t timeout, voidCallback_t cb) {
    clearDisplay();
    _display.setCursor(0, 0);
    _display.print(message);
    updateDisplay();
}

void DisplayMgr::showStartup() {
    clearDisplay();
    _display.setCursor(0, 0);
    _display.print("Car Radio");
    _display.setCursor(0, 1);
    _display.print("Starting...");
    updateDisplay();
}

void DisplayMgr::showInfo(time_t timeout) {
    clearDisplay();
    _display.setCursor(0, 0);
    _display.print("Info Screen");
    updateDisplay();
}

void DisplayMgr::showRadioChange() {
    clearDisplay();
    _display.setCursor(0, 0);
    _display.print("Radio Mode");
    updateDisplay();
}

void DisplayMgr::showScannerChange(bool force) {
    clearDisplay();
    _display.setCursor(0, 0);
    _display.print("Scanner Mode");
    updateDisplay();
}

// LED effect functions - implement as needed
void DisplayMgr::LEDeventStartup() {}
void DisplayMgr::LEDeventVol() {}
void DisplayMgr::LEDeventMute() {}
void DisplayMgr::LEDeventStop() {}
void DisplayMgr::LEDTunerUp(bool pinned) {}
void DisplayMgr::LEDTunerDown(bool pinned) {}
void DisplayMgr::LEDeventScannerStep() {}
void DisplayMgr::LEDeventScannerHold() {}
void DisplayMgr::LEDeventScannerStop() {}

DisplayMgr::mode_state_t DisplayMgr::active_mode() {
    return _mode;
}
