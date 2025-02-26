#include "U8G2_VFD.hpp"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
    printf("U8G2_VFD Test Program\n");
    
    // Create U8G2_VFD instance
    U8G2_VFD vfd(0x3C); // Default I2C address for SSD1306
    
    // Initialize the display
    if (!vfd.begin("/dev/i2c-1")) {
        printf("Failed to initialize display\n");
        return 1;
    }
    
    // Clear the screen
    vfd.clearScreen();
    sleep(1);
    
    // Test different fonts
    printf("Testing font 5x7\n");
    vfd.setFont(VFD::FONT_5x7);
    vfd.setCursor(0, 0);
    vfd.write("Font 5x7");
    sleep(2);
    
    printf("Testing font 10x14\n");
    vfd.setFont(VFD::FONT_10x14);
    vfd.setCursor(0, 20);
    vfd.write("Font 10x14");
    sleep(2);
    
    printf("Testing font mini\n");
    vfd.setFont(VFD::FONT_MINI);
    vfd.setCursor(0, 40);
    vfd.write("Font Mini");
    sleep(2);
    
    // Test cursor positioning
    vfd.clearScreen();
    vfd.setFont(VFD::FONT_5x7);
    
    vfd.setCursor(0, 0);
    vfd.write("Top Left");
    
    vfd.setCursor(vfd.width() - 50, 0);
    vfd.write("Top Right");
    
    vfd.setCursor(0, vfd.height() - 10);
    vfd.write("Bottom Left");
    
    vfd.setCursor(vfd.width() - 70, vfd.height() - 10);
    vfd.write("Bottom Right");
    
    sleep(3);
    
    // Test clear area
    printf("Testing clear area\n");
    uint8_t clearCmd[] = {VFD::VFD_CLEAR_AREA, 0, 20, 128, 40};
    vfd.writePacket(clearCmd, sizeof(clearCmd));
    
    sleep(1);
    
    // Test formatted print
    vfd.setFont(VFD::FONT_5x7);
    vfd.setCursor(10, 25);
    vfd.printPacket("Test %d: %s", 1, "Formatted Print");
    
    sleep(3);
    
    // Final clear
    vfd.clearScreen();
    
    printf("Test complete\n");
    return 0;
}
