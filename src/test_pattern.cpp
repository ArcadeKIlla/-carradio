#include "SH1106.hpp"
#include <unistd.h>

int main() {
    SH1106 display;
    
    if (!display.begin()) {
        printf("Failed to initialize display\n");
        return -1;
    }
    
    // Clear the display
    display.clear();
    
    // Draw a test pattern - checkerboard
    for (int y = 0; y < SH1106::DISPLAY_HEIGHT; y += 8) {
        for (int x = 0; x < SH1106::DISPLAY_WIDTH; x += 8) {
            bool color = ((x / 8) + (y / 8)) % 2 == 0;
            for (int dy = 0; dy < 8; dy++) {
                for (int dx = 0; dx < 8; dx++) {
                    display.drawPixel(x + dx, y + dy, color);
                }
            }
        }
    }
    
    // Draw a border
    for (int i = 0; i < SH1106::DISPLAY_WIDTH; i++) {
        display.drawPixel(i, 0, true);
        display.drawPixel(i, SH1106::DISPLAY_HEIGHT - 1, true);
    }
    for (int i = 0; i < SH1106::DISPLAY_HEIGHT; i++) {
        display.drawPixel(0, i, true);
        display.drawPixel(SH1106::DISPLAY_WIDTH - 1, i, true);
    }
    
    // Display everything
    display.display();
    
    printf("Test pattern displayed. Press Ctrl+C to exit.\n");
    while(1) {
        sleep(1);
    }
    
    return 0;
}
