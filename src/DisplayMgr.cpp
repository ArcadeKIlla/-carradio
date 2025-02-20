#include "DisplayMgr.hpp"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// RotaryEncoder pins
constexpr uint8_t ENCODER_CLK_PIN = 2;
constexpr uint8_t ENCODER_DT_PIN = 3;
constexpr uint8_t ENCODER_SW_PIN = 4;

DisplayMgr::DisplayMgr() 
    : _display(new SSD1306_LCD()),
      _encoder(new RotaryEncoder(ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN)),
      _mode(MODE_UNKNOWN) {
}

DisplayMgr::~DisplayMgr() {
    stop();
}

bool DisplayMgr::begin() {
    // Initialize display
    if (!_display->begin()) {
        printf("Failed to initialize display\n");
        return false;
    }

    // Initialize encoder
    if (!_encoder->begin()) {
        printf("Failed to initialize encoder\n");
        return false;
    }

    // Set up encoder callbacks
    _encoder->onRotate([this](int direction) {
        onEncoderRotate(direction);
    });

    _encoder->onButtonPress([this]() {
        onEncoderPress();
    });

    _encoder->onButtonLongPress([this]() {
        onEncoderLongPress();
    });

    _mode = MODE_STARTUP;
    showStartup();
    return true;
}

bool DisplayMgr::begin(const char* path, speed_t speed) {
    return begin();
}

bool DisplayMgr::begin(const char* path, speed_t speed, int& error) {
    return begin();
}

void DisplayMgr::stop() {
    if (_encoder) {
        _encoder->stop();
    }
    _mode = MODE_UNKNOWN;
    clear();
}

void DisplayMgr::clear() {
    _display->clear();
    _line1.clear();
    _line2.clear();
    update();
}

void DisplayMgr::setLine1(const std::string& text) {
    _line1 = text;
    _display->setCursor(0, 0);
    _display->print(text.c_str());
}

void DisplayMgr::setLine2(const std::string& text) {
    _line2 = text;
    _display->setCursor(0, 1);
    _display->print(text.c_str());
}

void DisplayMgr::update() {
    _display->display();
}

void DisplayMgr::showStartup() {
    clear();
    setLine1("Car Radio");
    setLine2("Starting...");
    update();
}

void DisplayMgr::showInfo() {
    clear();
    setLine1("Info Screen");
    update();
}

void DisplayMgr::showRadio() {
    clear();
    setLine1("Radio Mode");
    update();
}

void DisplayMgr::showScanner() {
    clear();
    setLine1("Scanner Mode");
    update();
}

void DisplayMgr::showTime() {
    clearDisplay();
    _display->setCursor(0, 0);
    _display->print("12:00");  // Replace with actual time
    updateDisplay();
}

void DisplayMgr::showMessage(string message, time_t timeout, voidCallback_t cb) {
    clearDisplay();
    _display->setCursor(0, 0);
    _display->print(message);
    updateDisplay();
}

void DisplayMgr::showRadioChange() {
    clearDisplay();
    _display->setCursor(0, 0);
    _display->print("Radio Mode");
    updateDisplay();
}

void DisplayMgr::showScannerChange(bool force) {
    clearDisplay();
    _display->setCursor(0, 0);
    _display->print("Scanner Mode");
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

void DisplayMgr::onEncoderRotate(int direction) {
    // Handle rotation - implement as needed
    printf("Encoder rotated: %d\n", direction);
}

void DisplayMgr::onEncoderPress() {
    // Handle button press - implement as needed
    printf("Encoder pressed\n");
}

void DisplayMgr::onEncoderLongPress() {
    // Handle long press - implement as needed
    printf("Encoder long pressed\n");
}
