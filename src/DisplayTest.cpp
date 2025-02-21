#include "SSD1306.hpp"
#include <stdio.h>
#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_MS(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

int main() {
    SSD1306 display;  // Uses default address 0x3C

    if (!display.begin()) {
        printf("Failed to initialize display\n");
        return 1;
    }

    // Clear the display
    display.clear();
    display.display();
    SLEEP_MS(1000);  // Wait 1 second

    // Draw some text
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("Car Radio");
    display.display();
    SLEEP_MS(2000);

    // Draw a rectangle
    display.drawRect(0, 16, 128, 16, true, true);
    display.display();
    SLEEP_MS(2000);

    // Draw some more text
    display.setCursor(0, 40);
    display.print("Testing 1-2-3");
    display.display();

    printf("Test complete\n");
    return 0;
}
