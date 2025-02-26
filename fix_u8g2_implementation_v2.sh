#!/bin/bash
# Script to fix U8G2 implementation issues - Version 2

# Exit on error
set -e

echo "=== U8G2 Implementation Fix Script (v2) ==="
echo "This script will fix the U8G2_VFD implementation issues"

# 1. Backup original files
echo "Step 1: Backing up original files..."
if [ -f "src/U8G2_VFD.hpp" ]; then
  cp "src/U8G2_VFD.hpp" "src/U8G2_VFD.hpp.bak"
  echo "Backed up src/U8G2_VFD.hpp to src/U8G2_VFD.hpp.bak"
fi

# 2. Copy fixed files
echo "Step 2: Copying fixed implementation files..."
cp "src/U8G2_VFD_fixed.hpp" "src/U8G2_VFD.hpp"
echo "Copied U8G2_VFD_fixed.hpp to U8G2_VFD.hpp"

# 3. Create the hardware I2C implementation file
echo "Step 3: Creating hardware I2C implementation..."
cat > "src/u8g2_hw_i2c.cpp" << 'EOL'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include "u8g2.h"

// Hardware I2C implementation for U8G2 on Linux
uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static int i2c_fd = -1;
    static uint8_t i2c_address;
    
    // Move variable declaration outside the switch statement
    const char* i2c_device = "/dev/i2c-1"; // Default
    
    switch(msg) {
        case U8X8_MSG_BYTE_INIT:
        {
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
            
        case U8X8_MSG_BYTE_SEND:
        {
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
            
        case U8X8_MSG_BYTE_SET_DC:
            // Not used for I2C
            return 1;
            
        default:
            return 0;
    }
}
EOL
echo "Created src/u8g2_hw_i2c.cpp"

# 4. Update CMakeLists.txt to include u8g2_hw_i2c.cpp
echo "Step 4: Updating CMakeLists.txt..."
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

# 5. Build the project
echo "Step 5: Building the project..."
mkdir -p build
cd build
cmake ..
make -j4

echo "U8G2 implementation fix complete!"
echo "If the build was successful, you can now run the application with ./carradio"
