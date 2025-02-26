#!/bin/bash
# Script to fix the U8G2 library path in CMakeLists.txt
# This script directly modifies the path without using sed

set -e  # Exit on error

echo "=== CMakeLists.txt U8G2 Path Fix Script ==="

# Check if CMakeLists.txt exists
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in the current directory."
    echo "Please run this script from the carradio project root directory."
    exit 1
fi

# Backup the original CMakeLists.txt
cp CMakeLists.txt CMakeLists.txt.bak
echo "Created backup of CMakeLists.txt as CMakeLists.txt.bak"

# Create a new CMakeLists.txt with the correct path
echo "Creating new CMakeLists.txt with correct U8G2 path..."

# Read the file line by line and replace the U8G2_DIR path
while IFS= read -r line; do
    if [[ "$line" == *"set(U8G2_DIR C:/Users/Nate/u8g2/csrc)"* ]]; then
        # Replace with the correct path
        echo "set(U8G2_DIR /home/nate/u8g2/csrc)"
    else
        # Keep the original line
        echo "$line"
    fi
done < CMakeLists.txt.bak > CMakeLists.txt.new

# Replace the original file with the new one
mv CMakeLists.txt.new CMakeLists.txt
echo "CMakeLists.txt has been updated with the correct U8G2 library path."

echo "Now make sure the U8G2 library is installed in the correct location:"
echo "  ls -la /home/nate/u8g2/csrc"

echo "If the directory doesn't exist, run:"
echo "  mkdir -p /home/nate/u8g2"
echo "  git clone https://github.com/olikraus/u8g2.git /tmp/u8g2"
echo "  cp -r /tmp/u8g2/csrc /home/nate/u8g2/"
echo "  rm -rf /tmp/u8g2"

echo "Then try building again:"
echo "  mkdir -p build"
echo "  cd build"
echo "  cmake .."
echo "  make -j4"
