#!/bin/bash
# Script to update the U8G2 library path in CMakeLists.txt
# This script should be run on the Raspberry Pi

set -e  # Exit on error

echo "=== CMakeLists.txt U8G2 Path Update Script ==="

# Check if CMakeLists.txt exists
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in the current directory."
    echo "Please run this script from the carradio project root directory."
    exit 1
fi

# Backup the original CMakeLists.txt
cp CMakeLists.txt CMakeLists.txt.bak
echo "Created backup of CMakeLists.txt as CMakeLists.txt.bak"

# Update the U8G2_DIR path
echo "Updating U8G2 library path in CMakeLists.txt..."
sed -i 's|set(U8G2_DIR C:/Users/Nate/u8g2/csrc)|set(U8G2_DIR ~/u8g2/csrc)|g' CMakeLists.txt

echo "CMakeLists.txt has been updated with the correct U8G2 library path."
echo "You can now build the project using:"
echo "  mkdir -p build"
echo "  cd build"
echo "  cmake .."
echo "  make -j4"
