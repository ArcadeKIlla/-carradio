# U8G2_VFD Implementation for CarRadio

This implementation provides a VFD interface using the U8G2 graphics library for OLED displays.

## Features

- Compatible with existing VFD interface
- Supports SSD1306 and SH1106 OLED displays
- Multiple font sizes (5x7, 10x14, Mini)
- Hardware I2C communication
- Cursor positioning and screen clearing
- Area clearing

## Usage

To use the U8G2_VFD in your code:

```cpp
#include "U8G2_VFD.hpp"

// Create a U8G2_VFD instance
U8G2_VFD vfd(0x3C); // Default I2C address for SSD1306

// Initialize the display
if (!vfd.begin("/dev/i2c-1")) {
    // Handle error
}

// Clear the screen
vfd.clearScreen();

// Set font and cursor position
vfd.setFont(VFD::FONT_5x7);
vfd.setCursor(0, 0);

// Write text
vfd.write("Hello, World!");
```

## Testing

A test program is provided to verify the functionality of the U8G2_VFD implementation.

To build and run the test program:

```bash
# Make the build script executable
chmod +x build_test_u8g2_vfd.sh

# Build the test program
./build_test_u8g2_vfd.sh

# Run the test program (requires sudo for I2C access)
cd build_test
sudo ./test_u8g2_vfd
```

## Integration with CarRadio

To use U8G2_VFD in the CarRadio project:

1. In `main.cpp`, replace the SSD1306_VFD instance with U8G2_VFD:

```cpp
// Replace:
// SSD1306_VFD vfd;

// With:
U8G2_VFD vfd;
```

2. The rest of the code should work as-is, since U8G2_VFD implements the same interface as SSD1306_VFD.

## Troubleshooting

- If the display doesn't initialize, check the I2C address and device path
- Use `i2cdetect -y 1` to verify the I2C address of your display
- Make sure the u8g2 library is properly included in the build

## Dependencies

- U8G2 library (included in the project)
- I2C.hpp (from the CarRadio project)
- Linux I2C and GPIO support
