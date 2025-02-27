#!/bin/bash
# Script to create a fresh U8G2 implementation for SSD1306 OLED display

# Exit on error
set -e

echo "=== Fresh U8G2 Implementation for SSD1306 OLED ==="
echo "This script will create a clean implementation of U8G2 for SSD1306 OLED display"

# 1. Create the OLED display adapter class
echo "Step 1: Creating OLED display adapter class..."
cat > "src/OLED_Display.hpp" << 'EOL'
#ifndef OLED_DISPLAY_HPP
#define OLED_DISPLAY_HPP

#include <string>
#include <memory>
#include "u8g2.h"

/**
 * @brief OLED Display adapter class for SSD1306 using U8G2 library
 * 
 * This class provides a simple interface to the SSD1306 OLED display
 * using the U8G2 library. It handles initialization, text display,
 * and basic graphics operations.
 */
class OLED_Display {
public:
    /**
     * @brief Construct a new OLED_Display object
     * 
     * @param i2c_bus I2C bus to use (e.g., "/dev/i2c-1")
     * @param address I2C address of the display (default: 0x3C)
     */
    OLED_Display(const std::string& i2c_bus = "/dev/i2c-1", uint8_t address = 0x3C);
    
    /**
     * @brief Destroy the OLED_Display object
     */
    ~OLED_Display();
    
    /**
     * @brief Initialize the display
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool init();
    
    /**
     * @brief Clear the display
     */
    void clear();
    
    /**
     * @brief Display text at the specified position
     * 
     * @param x X coordinate
     * @param y Y coordinate
     * @param text Text to display
     */
    void drawText(uint8_t x, uint8_t y, const std::string& text);
    
    /**
     * @brief Set the font to use
     * 
     * @param font Font to use (e.g., u8g2_font_ncenB08_tr)
     */
    void setFont(const uint8_t* font);
    
    /**
     * @brief Update the display
     * 
     * Call this after making changes to send them to the display
     */
    void updateDisplay();
    
    /**
     * @brief Get the U8G2 object
     * 
     * @return u8g2_t* Pointer to the U8G2 object
     */
    u8g2_t* getU8G2() { return &u8g2; }

private:
    u8g2_t u8g2;
    std::string i2c_bus;
    uint8_t address;
    bool initialized;
};

#endif // OLED_DISPLAY_HPP
EOL
echo "Created src/OLED_Display.hpp"

# 2. Create the implementation file
echo "Step 2: Creating OLED display adapter implementation..."
cat > "src/OLED_Display.cpp" << 'EOL'
#include "OLED_Display.hpp"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

// Hardware I2C callback for U8G2
static uint8_t u8x8_byte_hw_i2c_linux(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static int i2c_fd = -1;
    static uint8_t i2c_address;
    const char* i2c_device = "/dev/i2c-1"; // Default
    
    switch(msg) {
        case U8X8_MSG_BYTE_INIT: {
            // Initialize I2C
            if (i2c_fd >= 0) {
                close(i2c_fd);
            }
            
            // Get the I2C device path from u8x8 user_ptr (if available)
            if (u8x8->user_ptr != NULL) {
                i2c_device = (const char*)u8x8->user_ptr;
            }
            
            i2c_fd = open(i2c_device, O_RDWR);
            if (i2c_fd < 0) {
                perror("Failed to open I2C device");
                return 0;
            }
            
            // Get the I2C address from u8x8
            i2c_address = u8x8_GetI2CAddress(u8x8) >> 1; // U8G2 uses shifted address
            
            // Set the I2C slave address
            if (ioctl(i2c_fd, I2C_SLAVE, i2c_address) < 0) {
                perror("Failed to set I2C slave address");
                close(i2c_fd);
                i2c_fd = -1;
                return 0;
            }
            
            return 1;
        }
            
        case U8X8_MSG_BYTE_SEND: {
            // Send data to I2C device
            if (i2c_fd < 0) {
                return 0;
            }
            
            // Write data to I2C device
            if (write(i2c_fd, arg_ptr, arg_int) != arg_int) {
                perror("Failed to write to I2C device");
                return 0;
            }
            
            return 1;
        }
            
        case U8X8_MSG_BYTE_START_TRANSFER:
            // Nothing to do for I2C start transfer
            return 1;
            
        case U8X8_MSG_BYTE_END_TRANSFER:
            // Nothing to do for I2C end transfer
            return 1;
            
        default:
            return 0;
    }
}

// GPIO callback for U8G2 (minimal implementation)
static uint8_t u8x8_gpio_and_delay_linux(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            // Initialize GPIO and delay
            return 1;
            
        case U8X8_MSG_DELAY_MILLI:
            // Delay for arg_int milliseconds
            usleep(arg_int * 1000);
            return 1;
            
        case U8X8_MSG_GPIO_DC:
        case U8X8_MSG_GPIO_RESET:
        case U8X8_MSG_GPIO_CS:
        case U8X8_MSG_GPIO_CLOCK:
        case U8X8_MSG_GPIO_DATA:
            // Not used for I2C
            return 1;
    }
    
    return 0;
}

OLED_Display::OLED_Display(const std::string& i2c_bus, uint8_t address)
    : i2c_bus(i2c_bus), address(address), initialized(false) {
    // Initialize U8G2 structure
    memset(&u8g2, 0, sizeof(u8g2_t));
}

OLED_Display::~OLED_Display() {
    if (initialized) {
        u8g2_SetPowerSave(&u8g2, 1); // Put display in sleep mode
    }
}

bool OLED_Display::init() {
    // Initialize U8G2 for SSD1306 128x64 OLED display with hardware I2C
    char* bus_copy = new char[i2c_bus.length() + 1];
    strcpy(bus_copy, i2c_bus.c_str());
    
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_hw_i2c_linux,
        u8x8_gpio_and_delay_linux
    );
    
    // Set I2C address and bus
    u8g2_SetI2CAddress(&u8g2, address << 1); // U8G2 uses shifted address
    u8g2_SetUserPtr(&u8g2, bus_copy);
    
    // Initialize display
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0); // Wake up display
    u8g2_ClearBuffer(&u8g2);
    
    initialized = true;
    return true;
}

void OLED_Display::clear() {
    if (!initialized) return;
    
    u8g2_ClearBuffer(&u8g2);
}

void OLED_Display::drawText(uint8_t x, uint8_t y, const std::string& text) {
    if (!initialized) return;
    
    u8g2_DrawStr(&u8g2, x, y, text.c_str());
}

void OLED_Display::setFont(const uint8_t* font) {
    if (!initialized) return;
    
    u8g2_SetFont(&u8g2, font);
}

void OLED_Display::updateDisplay() {
    if (!initialized) return;
    
    u8g2_SendBuffer(&u8g2);
}
EOL
echo "Created src/OLED_Display.cpp"

# 3. Create a simple test program
echo "Step 3: Creating a simple test program..."
cat > "src/oled_test.cpp" << 'EOL'
#include "OLED_Display.hpp"
#include <iostream>
#include <unistd.h>

int main() {
    std::cout << "OLED Display Test" << std::endl;
    
    // Create OLED display object
    OLED_Display oled("/dev/i2c-1", 0x3C);
    
    // Initialize display
    if (!oled.init()) {
        std::cerr << "Failed to initialize OLED display" << std::endl;
        return 1;
    }
    
    std::cout << "OLED display initialized" << std::endl;
    
    // Set font
    oled.setFont(u8g2_font_ncenB08_tr);
    
    // Display text
    oled.clear();
    oled.drawText(0, 10, "Hello, World!");
    oled.drawText(0, 20, "OLED Test");
    oled.drawText(0, 30, "SSD1306 128x64");
    oled.updateDisplay();
    
    std::cout << "Text displayed" << std::endl;
    
    // Wait for 5 seconds
    sleep(5);
    
    // Display some more text
    oled.clear();
    oled.drawText(0, 10, "CarRadio");
    oled.drawText(0, 20, "Display Test");
    oled.drawText(0, 30, "U8G2 Library");
    oled.drawText(0, 40, "Raspberry Pi");
    oled.updateDisplay();
    
    std::cout << "More text displayed" << std::endl;
    
    // Wait for 5 seconds
    sleep(5);
    
    // Clear display
    oled.clear();
    oled.updateDisplay();
    
    std::cout << "Display cleared" << std::endl;
    
    return 0;
}
EOL
echo "Created src/oled_test.cpp"

# 4. Create a simple CMakeLists.txt for the test program
echo "Step 4: Creating CMakeLists.txt for the test program..."
cat > "oled_test_CMakeLists.txt" << 'EOL'
cmake_minimum_required(VERSION 3.10)
project(oled_test)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Add U8G2 library
set(U8G2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/u8g2")
include_directories(${U8G2_DIR}/csrc)

# Collect U8G2 source files
file(GLOB U8G2_SOURCES ${U8G2_DIR}/csrc/*.c)

# Add source files
set(SOURCES
    src/OLED_Display.cpp
    src/oled_test.cpp
    ${U8G2_SOURCES}
)

# Create executable
add_executable(oled_test ${SOURCES})

# Link libraries
target_link_libraries(oled_test)

# Installation
install(TARGETS oled_test DESTINATION bin)
EOL
echo "Created oled_test_CMakeLists.txt"

# 5. Create instructions for building and testing
echo "Step 5: Creating build and test instructions..."
cat > "OLED_DISPLAY_README.md" << 'EOL'
# OLED Display Implementation with U8G2

This is a clean implementation of an OLED display adapter using the U8G2 library for the CarRadio project.

## Prerequisites

- Raspberry Pi with I2C enabled
- SSD1306 OLED display connected via I2C
- U8G2 library

## Setup

1. Clone the U8G2 library if you haven't already:
   ```bash
   git clone https://github.com/olikraus/u8g2.git
   ```

2. Make sure I2C is enabled on your Raspberry Pi:
   ```bash
   sudo raspi-config
   # Navigate to Interface Options > I2C > Enable
   ```

3. Install I2C development libraries:
   ```bash
   sudo apt-get update
   sudo apt-get install -y libi2c-dev i2c-tools
   ```

4. Check if your OLED display is detected:
   ```bash
   i2cdetect -y 1
   ```
   You should see your device (typically at address 0x3C or 0x3D).

## Building the Test Program

1. Use the provided CMakeLists.txt for the test program:
   ```bash
   cp oled_test_CMakeLists.txt CMakeLists.txt
   ```

2. Create a build directory and build the test program:
   ```bash
   mkdir -p build
   cd build
   cmake ..
   make
   ```

3. Run the test program:
   ```bash
   ./oled_test
   ```

## Integrating with CarRadio

To integrate this OLED display adapter with the CarRadio project:

1. Add the OLED_Display.hpp and OLED_Display.cpp files to your project.

2. Include the U8G2 library in your build system.

3. Create an instance of the OLED_Display class in your code:
   ```cpp
   #include "OLED_Display.hpp"
   
   // Create OLED display object
   OLED_Display oled("/dev/i2c-1", 0x3C);
   
   // Initialize display
   if (!oled.init()) {
       // Handle error
   }
   
   // Use the display
   oled.clear();
   oled.drawText(0, 10, "Hello, World!");
   oled.updateDisplay();
   ```

## Troubleshooting

- **Display not found**: Check your I2C connections and address using `i2cdetect -y 1`.
- **Build errors**: Make sure you have the U8G2 library in the correct location.
- **Display not showing anything**: Check power connections and contrast settings.

## Customization

- **Different display type**: Modify the `u8g2_Setup_*` function call in OLED_Display.cpp.
- **Different I2C bus**: Pass a different bus path to the constructor.
- **Different I2C address**: Pass a different address to the constructor.

## References

- [U8G2 Library Documentation](https://github.com/olikraus/u8g2/wiki)
- [SSD1306 Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
EOL
echo "Created OLED_DISPLAY_README.md"

echo "Fresh U8G2 implementation created!"
echo "Follow the instructions in OLED_DISPLAY_README.md to build and test the implementation."
echo "To use this implementation with your CarRadio project, copy the OLED_Display.hpp and OLED_Display.cpp files to your project and update your CMakeLists.txt accordingly."
