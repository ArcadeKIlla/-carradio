#!/bin/bash
# Script to deploy U8G2 library for the CarRadio project
# This script should be run on the Raspberry Pi

set -e  # Exit on error

echo "=== U8G2 Library Deployment Script ==="
echo "This script will install the U8G2 library for the CarRadio project"

# Create temporary directory
TEMP_DIR=$(mktemp -d)
echo "Created temporary directory: $TEMP_DIR"

# Clone U8G2 repository
echo "Cloning U8G2 repository..."
git clone https://github.com/olikraus/u8g2.git "$TEMP_DIR/u8g2"

# Create U8G2 directory if it doesn't exist
mkdir -p ~/u8g2

# Copy the csrc directory
echo "Copying U8G2 source files..."
cp -r "$TEMP_DIR/u8g2/csrc" ~/u8g2/

# Clean up
echo "Cleaning up temporary files..."
rm -rf "$TEMP_DIR"

echo "U8G2 library has been successfully deployed to ~/u8g2/csrc"
echo ""
echo "To build the CarRadio project, use the following commands:"
echo "  cd ~/carradio"
echo "  mkdir -p build"
echo "  cd build"
echo "  cmake .."
echo "  make -j4"
echo ""
echo "If you encounter any issues, make sure the CMakeLists.txt file has the correct path to the U8G2 library."
