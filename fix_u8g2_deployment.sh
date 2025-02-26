#!/bin/bash
# Comprehensive script to fix U8G2 deployment issues
# This script will:
# 1. Fix the CMakeLists.txt file
# 2. Install the U8G2 library in the correct location
# 3. Build the project

set -e  # Exit on error

echo "=== U8G2 Deployment Fix Script ==="
echo "This script will fix the U8G2 library deployment issues"

# Check if we're in the carradio directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in the current directory."
    echo "Please run this script from the carradio project root directory."
    exit 1
fi

# Step 1: Fix the CMakeLists.txt file
echo "Step 1: Fixing CMakeLists.txt..."
cp CMakeLists.txt CMakeLists.txt.bak
echo "Created backup of CMakeLists.txt as CMakeLists.txt.bak"

# Directly edit the file to replace the Windows path with the Linux path
echo "Updating U8G2 library path in CMakeLists.txt..."
# Use awk for more reliable text replacement
awk '{
    if ($0 ~ /set\(U8G2_DIR C:\/Users\/Nate\/u8g2\/csrc\)/) {
        print "set(U8G2_DIR /home/nate/u8g2/csrc)"
    } else {
        print $0
    }
}' CMakeLists.txt.bak > CMakeLists.txt.new

# Replace the original file with the new one
mv CMakeLists.txt.new CMakeLists.txt
echo "CMakeLists.txt has been updated with the correct U8G2 library path."

# Step 2: Install the U8G2 library
echo "Step 2: Installing U8G2 library..."
mkdir -p /home/nate/u8g2

# Check if the library is already installed
if [ -d "/home/nate/u8g2/csrc" ]; then
    echo "U8G2 library is already installed at /home/nate/u8g2/csrc"
else
    echo "Cloning U8G2 repository..."
    git clone https://github.com/olikraus/u8g2.git /tmp/u8g2
    
    echo "Copying U8G2 source files..."
    cp -r /tmp/u8g2/csrc /home/nate/u8g2/
    
    echo "Cleaning up temporary files..."
    rm -rf /tmp/u8g2
    
    echo "U8G2 library has been installed at /home/nate/u8g2/csrc"
fi

# Step 3: Build the project
echo "Step 3: Building the project..."
mkdir -p build
cd build
echo "Running cmake..."
cmake ..
echo "Running make..."
make -j4

echo "Build completed. If successful, you can run the application with:"
echo "  sudo ./carradio"
echo ""
echo "If you still encounter issues, please check:"
echo "1. That the U8G2 library is properly installed at /home/nate/u8g2/csrc"
echo "2. That the CMakeLists.txt file has the correct path: set(U8G2_DIR /home/nate/u8g2/csrc)"
echo "3. That all other dependencies are installed"
