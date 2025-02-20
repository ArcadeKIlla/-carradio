#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "RotaryEncoder.hpp"
#include "SSD1306_LCD.hpp"
#include "RadioMgr.hpp"
#include "RGB.hpp"

using namespace std;

class DisplayMgr {
public:
    DisplayMgr();
    ~DisplayMgr();

    bool begin(const char* path, speed_t speed = B19200);
    bool begin(const char* path, speed_t speed, int& error);
    void stop();
    bool reset();

    // LED effects
    void LEDeventStartup();
    void LEDeventVol();
    void LEDeventMute();
    void LEDeventStop();
    void LEDTunerUp(bool pinned = false);
    void LEDTunerDown(bool pinned = false);
    void LEDeventScannerStep();
    void LEDeventScannerHold();
    void LEDeventScannerStop();

    // active mode
    typedef enum {
        MODE_UNKNOWN = 0,
        MODE_NOCHANGE,    // no change in mode
        MODE_STARTUP,
        MODE_TIME,
        MODE_DIMMER,
        MODE_SQUELCH,
        MODE_RADIO,
        MODE_SCANNER,
        MODE_CANBUS,
        MODE_GPS,
        MODE_GPS_WAYPOINTS,
        MODE_GPS_WAYPOINT,
        MODE_DTC,
        MODE_DTC_INFO,
        MODE_INFO,
        MODE_DEV_STATUS,
        MODE_MENU,
        MODE_MESSAGE,
        MODE_EDIT_STRING,
        MODE_SCANNER_CHANNELS,
        MODE_CHANNEL_INFO,
        MODE_SLIDER,
        MODE_SELECT_SLIDER
    } mode_state_t;

    mode_state_t active_mode();

    // knobs
    typedef enum {
        KNOB_EXIT = 0,
        KNOB_UP,
        KNOB_DOWN,
        KNOB_CLICK,
        KNOB_DOUBLE_CLICK,
        KNOB_SELECTING     // special callback for things like scanner channels
    } knob_action_t;

    typedef enum {
        KNOB_RIGHT,
        KNOB_LEFT,
    } knob_id_t;

    RotaryEncoder* rightKnob() { return _rightKnob.get(); };
    RotaryEncoder* leftKnob() { return _leftKnob.get(); };

    // display related
    bool setBrightness(double level);   // 0.0 -  1.0
    bool setKnobBackLight(bool isOn);
    bool setKnobColor(knob_id_t, RGB);

    // Display functions
    void showTime();
    void showMessage(string message = "", time_t timeout = 0, voidCallback_t cb = nullptr);
    void showStartup();
    void showInfo(time_t timeout = 0);
    void showDTC();
    void showDTCInfo(string code);
    void showDimmerChange();
    void showBalanceChange();
    void showFaderChange();
    void showSquelchChange();
    void showRadioChange();
    void showScannerChange(bool force = true);
    void showAirplayChange();
    void showCANbus(uint8_t page = 0);
    void showDevStatus();

private:
    std::unique_ptr<SSD1306_LCD> _display;
    std::unique_ptr<RotaryEncoder> _rightKnob;
    std::unique_ptr<RotaryEncoder> _leftKnob;
    mode_state_t _mode;
    bool _shouldAutoPlay;

    void updateDisplay();
    void clearDisplay();
};
