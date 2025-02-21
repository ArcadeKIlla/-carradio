#include "SSD1306_VFD.hpp"
#include <iostream>
#include <unistd.h>

int main() {
    // Create OLED display adapter with default I2C address (0x3C)
    SSD1306_VFD display;
    
    // Initialize display
    std::cout << "Initializing display..." << std::endl;
    if (!display.begin("/dev/i2c-1", B9600)) {
        std::cerr << "Failed to initialize display!" << std::endl;
        return 1;
    }
    
    // Clear display
    display.clearScreen();
    
    // Set font size
    display.setFont(VFD::FONT_10x14);
    
    // Write some test text
    display.setCursor(0, 0);
    display.write("OLED Test");
    
    display.setFont(VFD::FONT_MINI);
    display.setCursor(0, 20);
    display.write("Hello World!");
    display.setCursor(0, 30);
    display.write("Line 2");
    
    // Draw a rectangle
    display.drawScrollBar(40, 0.5, 0.2);
    
    std::cout << "Test complete. Check the display." << std::endl;
    sleep(5);
    
    return 0;
}
