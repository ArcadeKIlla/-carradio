#include "SSD1306.hpp"
#include <stdio.h>
#include <windows.h>

int main() {
    SSD1306 display;  // Uses default address 0x3C

    if (!display.begin()) {
        printf("Failed to initialize display\n");
        return 1;
    }

    // Clear the display
    display.clear();
    display.display();
    Sleep(1000);  // Wait 1 second

    // Draw some text
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("Car Radio");
    display.display();
    Sleep(2000);

    // Draw a rectangle
    display.drawRect(0, 16, 128, 16, true, true);
    display.display();
    Sleep(2000);

    // Draw some more text
    display.setCursor(0, 40);
    display.print("Testing 1-2-3");
    display.display();

    printf("Test complete\n");
    return 0;
}
