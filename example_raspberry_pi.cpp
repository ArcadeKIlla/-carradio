//
//  example_raspberry_pi.cpp
//  carradio_v2_sh1106
//
//  Created on 2/26/2025
//  Example of using the VFD_SH1106_Adapter on Raspberry Pi
//

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include "VFD_SH1106_Adapter.hpp"

int main() {
    // Create the adapter
    VFD_SH1106_Adapter* display = new VFD_SH1106_Adapter();
    
    // Initialize with I2C on Raspberry Pi
    // /dev/i2c-1 is the default I2C bus on most Raspberry Pi models
    // 0x3C is the default I2C address for most SH1106 displays
    if (!display->begin("/dev/i2c-1", 0x3C)) {
        std::cerr << "Failed to initialize display!" << std::endl;
        delete display;
        return 1;
    }
    
    std::cout << "Display initialized successfully." << std::endl;
    
    // Clear the screen
    display->clearScreen();
    
    // Set maximum brightness
    display->setBrightness(7);
    
    // Display a simple message
    display->setCursor(0, 10);
    display->setFont(VFD_SH1106_Adapter::FONT_5x7);
    display->write("CarRadio");
    
    display->setCursor(0, 25);
    display->setFont(VFD_SH1106_Adapter::FONT_MINI);
    display->write("SH1106 Adapter");
    
    display->setCursor(0, 40);
    display->write("Running on Raspberry Pi");
    
    // Draw a scroll bar
    display->drawScrollBar(0, 0.5, 0.25);
    
    // Display multiple lines
    std::vector<std::string> lines = {
        "Line 1",
        "Line 2",
        "Line 3",
        "Line 4"
    };
    
    sleep(2); // Wait 2 seconds
    display->clearScreen();
    display->printLines(10, 10, lines, 0, 4, VFD_SH1106_Adapter::FONT_MINI);
    
    // Display rows with columns
    std::vector<std::vector<std::string>> columns = {
        {"Item 1", "Value 1"},
        {"Item 2", "Value 2"},
        {"Item 3", "Value 3"}
    };
    
    sleep(2); // Wait 2 seconds
    display->clearScreen();
    display->printRows(10, 10, columns, 0, 3, 0, VFD_SH1106_Adapter::FONT_MINI);
    
    // Clean up
    sleep(5); // Keep the display on for 5 seconds
    display->setPowerOn(false);
    delete display;
    
    return 0;
}
