#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "SH1106.hpp"

int main() {
    printf("Starting OLED test program\n");
    
    // Initialize the OLED display
    SH1106 display;
    if (!display.begin()) {
        printf("Failed to initialize OLED display\n");
        return 1;
    }
    
    printf("OLED display initialized\n");
    
    // Clear the display
    display.clear();
    display.display();
    
    printf("Drawing a test pattern...\n");
    
    // Draw a pattern
    // 1. A border around the display
    for (int i = 0; i < SH1106::DISPLAY_WIDTH; i++) {
        display.drawPixel(i, 0, true);
        display.drawPixel(i, SH1106::DISPLAY_HEIGHT-1, true);
    }
    for (int i = 0; i < SH1106::DISPLAY_HEIGHT; i++) {
        display.drawPixel(0, i, true);
        display.drawPixel(SH1106::DISPLAY_WIDTH-1, i, true);
    }
    
    // 2. Diagonal lines
    display.drawLine(0, 0, SH1106::DISPLAY_WIDTH-1, SH1106::DISPLAY_HEIGHT-1, true);
    display.drawLine(0, SH1106::DISPLAY_HEIGHT-1, SH1106::DISPLAY_WIDTH-1, 0, true);
    
    // 3. Draw a rectangle in the middle
    display.drawRect(SH1106::DISPLAY_WIDTH/4, SH1106::DISPLAY_HEIGHT/4, 
                     SH1106::DISPLAY_WIDTH/2, SH1106::DISPLAY_HEIGHT/2, 
                     false, true);
    
    // Update the display
    printf("Sending to display...\n");
    display.display();
    printf("Test pattern sent to display\n");
    
    // Wait for 2 seconds
    sleep(2);
    
    // Display some text
    printf("Drawing text...\n");
    display.clear();
    display.setCursor(0, 0);
    display.print("OLED TEST");
    display.setCursor(0, 16);
    display.print("LINE 2");
    display.setCursor(0, 32);
    display.print("LINE 3");
    display.setCursor(0, 48);
    display.print("LINE 4");
    display.display();
    
    printf("Text sent to display\n");
    printf("Test complete, press Ctrl+C to exit\n");
    
    // Keep the program running
    while(1) {
        sleep(1);
    }
    
    return 0;
}
