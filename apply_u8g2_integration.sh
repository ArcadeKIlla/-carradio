#!/bin/bash

# Apply the U8G2 integration patch
echo "Applying U8G2 integration patch..."
patch -p0 < u8g2_integration.patch

# Modify PiCarMgr.cpp to use U8G2_OLED_DISPLAY
echo "Updating PiCarMgr.cpp to use U8G2_OLED_DISPLAY..."
sed -i 's/_display(DisplayMgr::OLED_DISPLAY)/_display(DisplayMgr::U8G2_OLED_DISPLAY)/' src/PiCarMgr.cpp

# Build the project
echo "Building the project..."
mkdir -p build
cd build
cmake ..
make -j4

echo "U8G2 integration complete!"
