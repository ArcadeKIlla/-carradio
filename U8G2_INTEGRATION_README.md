# U8G2 Display Integration

This document explains how to integrate the U8G2 display library into the CarRadio project.

## What's Included

1. **U8G2_VFD.hpp**: The adapter class that implements the VFD interface using the U8G2 library
2. **u8g2_hw_i2c.cpp**: Hardware I2C communication layer for the U8G2 library
3. **Integration patch**: Patch file to update the DisplayMgr.cpp file
4. **Integration script**: Shell script to apply the patch and update PiCarMgr.cpp

## How to Apply the Integration

### On Raspberry Pi

1. Copy all the files to your Raspberry Pi:
   ```bash
   # From your development machine
   scp u8g2_integration.patch apply_u8g2_integration.sh pi@your-pi-ip:~/carradio/
   ```

2. SSH into your Raspberry Pi:
   ```bash
   ssh pi@your-pi-ip
   ```

3. Navigate to the carradio directory and run the integration script:
   ```bash
   cd ~/carradio
   chmod +x apply_u8g2_integration.sh
   ./apply_u8g2_integration.sh
   ```

4. Test the integration:
   ```bash
   cd ~/carradio
   sudo ./build/carradio
   ```

## Manual Integration

If you prefer to manually integrate the U8G2 display:

1. Modify `PiCarMgr.cpp`:
   - Change `_display(DisplayMgr::OLED_DISPLAY)` to `_display(DisplayMgr::U8G2_OLED_DISPLAY)` in the constructor

2. Modify `DisplayMgr.cpp`:
   - Add handling for U8G2_OLED_DISPLAY in the begin method

3. Build the project:
   ```bash
   cd ~/carradio
   mkdir -p build
   cd build
   cmake ..
   make -j4
   ```

## Benefits of U8G2 Library

- Better graphics support with drawing primitives
- Multiple font sizes and styles
- Improved memory management
- Support for many display types
- Hardware acceleration where available
- Comprehensive documentation

## Troubleshooting

If you encounter issues:

1. Check I2C connection and address (default is 0x3C)
2. Verify that the U8G2 library is properly installed
3. Check the display type in DisplayMgr.hpp
4. Ensure the display is properly initialized in PiCarMgr.cpp

For more information, see the U8G2_VFD_README.md file.
