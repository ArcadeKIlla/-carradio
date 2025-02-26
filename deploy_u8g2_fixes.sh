#!/bin/bash
# Script to deploy the U8G2 VFD fixes to a Raspberry Pi

# Exit on error
set -e

# Check if project directory is provided
if [ -z "$1" ]; then
  echo "Usage: $0 <project_directory>"
  echo "Example: $0 /home/pi/carradio"
  exit 1
fi

PROJECT_DIR="$1"
cd "$PROJECT_DIR"

echo "Deploying U8G2 VFD fixes to $PROJECT_DIR..."

# 1. Backup original files
echo "Backing up original files..."
if [ -f "src/U8G2_VFD.hpp" ]; then
  cp "src/U8G2_VFD.hpp" "src/U8G2_VFD.hpp.bak"
  echo "Backed up src/U8G2_VFD.hpp to src/U8G2_VFD.hpp.bak"
fi

# 2. Copy fixed files
echo "Copying fixed files..."
cp "src/U8G2_VFD_fixed.hpp" "src/U8G2_VFD.hpp"
cp "src/u8g2_hw_i2c_fixed.cpp" "src/u8g2_hw_i2c.cpp"
echo "Fixed files copied successfully."

# 3. Update CMakeLists.txt to include u8g2_hw_i2c.cpp
echo "Updating CMakeLists.txt..."
if ! grep -q "u8g2_hw_i2c.cpp" "CMakeLists.txt"; then
  # Find the line with the last source file
  LAST_SRC_LINE=$(grep -n "src/.*\.cpp" "CMakeLists.txt" | tail -1 | cut -d: -f1)
  if [ -n "$LAST_SRC_LINE" ]; then
    # Insert the new source file after the last source file
    sed -i "${LAST_SRC_LINE}a\\  src/u8g2_hw_i2c.cpp" "CMakeLists.txt"
    echo "Added src/u8g2_hw_i2c.cpp to CMakeLists.txt"
  else
    echo "Could not find source files in CMakeLists.txt. Please add src/u8g2_hw_i2c.cpp manually."
  fi
else
  echo "src/u8g2_hw_i2c.cpp already in CMakeLists.txt"
fi

# 4. Install required packages
echo "Installing required packages..."
sudo apt-get update
sudo apt-get install -y libi2c-dev i2c-tools

# 5. Check if U8G2 library is installed
echo "Checking for U8G2 library..."
if [ ! -f "/usr/local/include/u8g2.h" ]; then
  echo "U8G2 library not found. Installing..."
  
  # Clone U8G2 library
  if [ ! -d "u8g2" ]; then
    git clone https://github.com/olikraus/u8g2.git
  fi
  
  # Install U8G2 library
  cd u8g2/csrc
  sudo cp *.h /usr/local/include/
  sudo cp *.c /usr/local/src/
  cd "$PROJECT_DIR"
  
  echo "U8G2 library installed successfully."
else
  echo "U8G2 library already installed."
fi

# 6. Enable I2C if not already enabled
echo "Checking I2C status..."
if ! lsmod | grep -q "^i2c_bcm2835"; then
  echo "I2C not enabled. Please enable I2C using raspi-config:"
  echo "sudo raspi-config"
  echo "Navigate to: Interface Options > I2C > Enable"
else
  echo "I2C already enabled."
fi

# 7. Build the project
echo "Building the project..."
mkdir -p build
cd build
cmake ..
make

echo "Deployment complete!"
echo "You can now run the application with ./carradio"
echo ""
echo "To test the I2C display, run: sudo i2cdetect -y 1"
echo "This should show your display at address 0x3C (or your configured address)"
