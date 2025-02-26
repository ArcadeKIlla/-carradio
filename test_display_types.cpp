#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <iostream>

#include "DisplayMgr.hpp"
#include "DuppaEncoder.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    printf("Display Type Test\n");
    
    // Parse command line arguments
    DisplayMgr::DisplayType displayType = DisplayMgr::OLED_DISPLAY; // Default
    
    if (argc > 1) {
        string arg = argv[1];
        if (arg == "vfd") {
            displayType = DisplayMgr::VFD_DISPLAY;
            printf("Using VFD display\n");
        } else if (arg == "oled") {
            displayType = DisplayMgr::OLED_DISPLAY;
            printf("Using SSD1306 OLED display\n");
        } else if (arg == "u8g2") {
            displayType = DisplayMgr::U8G2_OLED_DISPLAY;
            printf("Using U8G2 OLED display\n");
        } else {
            printf("Unknown display type: %s\n", arg.c_str());
            printf("Valid options: vfd, oled, u8g2\n");
            return 1;
        }
    } else {
        printf("No display type specified, using default (OLED)\n");
        printf("Usage: %s [vfd|oled|u8g2]\n", argv[0]);
    }
    
    // Set up encoder configurations
    DisplayMgr::EncoderConfig leftConfig;
    leftConfig.type = DisplayMgr::GENERIC_ENCODER;
    leftConfig.generic.clkPin = 17;  // GPIO pin numbers
    leftConfig.generic.dtPin = 27;
    leftConfig.generic.swPin = 22;
    
    DisplayMgr::EncoderConfig rightConfig;
    rightConfig.type = DisplayMgr::GENERIC_ENCODER;
    rightConfig.generic.clkPin = 5;
    rightConfig.generic.dtPin = 6;
    rightConfig.generic.swPin = 13;
    
    // Create display manager
    DisplayMgr displayMgr(displayType, leftConfig, rightConfig);
    
    // Initialize display
    int error = 0;
    if (!displayMgr.begin("/dev/i2c-1", B400000, error)) {
        printf("Failed to initialize display: error %d\n", error);
        return 1;
    }
    
    // Show startup screen
    displayMgr.showStartup();
    sleep(2);
    
    // Show message
    displayMgr.showMessage("Display Test", 3, nullptr);
    sleep(3);
    
    // Show time
    displayMgr.showTime();
    sleep(2);
    
    // Show info
    displayMgr.showInfo(5);
    sleep(5);
    
    // Show device status
    displayMgr.showDevStatus();
    sleep(3);
    
    // Clean up
    displayMgr.stop();
    
    printf("Test complete\n");
    return 0;
}
