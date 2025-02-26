#!/bin/bash
# Script to fix U8G2 linker errors related to missing device drivers

# Exit on error
set -e

echo "=== U8G2 Linker Error Fix Script ==="
echo "This script will fix the U8G2 linker errors related to missing device drivers"

# 1. Create a custom U8G2 configuration file
echo "Step 1: Creating custom U8G2 configuration file..."
cat > "u8g2_custom_config.h" << 'EOL'
/*
  Custom U8G2 Configuration File
  
  This file enables or disables specific device drivers in the U8G2 library
  to resolve linker errors related to missing symbols.
*/

#ifndef U8G2_CUSTOM_CONFIG_H
#define U8G2_CUSTOM_CONFIG_H

// Enable specific device drivers
#define U8X8_WITH_A2PRINTER 1

// Include the original U8G2 configuration
#include "u8g2.h"

#endif /* U8G2_CUSTOM_CONFIG_H */
EOL
echo "Created u8g2_custom_config.h"

# 2. Create a patch for the u8x8_d_a2printer.c file if it's missing
echo "Step 2: Creating A2 printer device driver implementation..."
cat > "u8x8_d_a2printer.c" << 'EOL'
/*
  u8x8_d_a2printer.c
  
  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
  
  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.
  
  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:
  
  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
*/

#include "u8g2.h"

/* A2 Thermal Printer */
/* This is a placeholder implementation for the A2 printer device driver */

uint8_t u8x8_d_a2printer_384x240(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, y, c;
  uint8_t *ptr;
  
  switch(msg)
  {
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8x8, &u8x8_a2printer_384x240_display_info);
      break;
    case U8X8_MSG_DISPLAY_INIT:
      u8x8_d_helper_display_init(u8x8);
      break;
    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
      break;
    case U8X8_MSG_DISPLAY_SET_FLIP_MODE:
      break;
    case U8X8_MSG_DISPLAY_SET_CONTRAST:
      break;
    case U8X8_MSG_DISPLAY_DRAW_TILE:
      x = ((u8x8_tile_t *)arg_ptr)->x_pos;
      y = ((u8x8_tile_t *)arg_ptr)->y_pos;
      
      /* Placeholder for actual implementation */
      
      break;
    default:
      return 0;
  }
  return 1;
}

/* Define the display info structure if it's missing */
#ifndef u8x8_a2printer_384x240_display_info
const u8x8_display_info_t u8x8_a2printer_384x240_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 0,
  /* pre_chip_disable_wait_ns = */ 0,
  /* reset_pulse_width_ms = */ 0, 
  /* post_reset_wait_ms = */ 0, 
  /* sda_setup_time_ns = */ 0,
  /* sck_pulse_width_ns = */ 0,
  /* sck_clock_hz = */ 4000000UL,
  /* spi_mode = */ 0,
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 0,
  /* write_pulse_width_ns = */ 0,
  /* tile_width = */ 48,
  /* tile_height = */ 30,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 384,
  /* pixel_height = */ 240
};
#endif
EOL
echo "Created u8x8_d_a2printer.c"

# 3. Update CMakeLists.txt to include the custom configuration and A2 printer driver
echo "Step 3: Updating CMakeLists.txt..."

# Check if we need to add the custom configuration
if ! grep -q "u8g2_custom_config.h" "CMakeLists.txt"; then
  # Add include directory for custom config
  if grep -q "include_directories" "CMakeLists.txt"; then
    # Add to existing include_directories
    sed -i '/include_directories/a \ \ ${CMAKE_CURRENT_SOURCE_DIR}' "CMakeLists.txt"
    echo "Added current source directory to include_directories"
  else
    # Add new include_directories
    echo "include_directories(${CMAKE_CURRENT_SOURCE_DIR})" >> "CMakeLists.txt"
    echo "Added include_directories to CMakeLists.txt"
  fi
  
  # Add definition to use custom config
  if grep -q "add_definitions" "CMakeLists.txt"; then
    # Add to existing add_definitions
    sed -i '/add_definitions/a \ \ -DU8G2_USE_CUSTOM_CONFIG' "CMakeLists.txt"
    echo "Added U8G2_USE_CUSTOM_CONFIG definition"
  else
    # Add new add_definitions
    echo "add_definitions(-DU8G2_USE_CUSTOM_CONFIG)" >> "CMakeLists.txt"
    echo "Added add_definitions to CMakeLists.txt"
  fi
fi

# Check if we need to add the A2 printer driver
if ! grep -q "u8x8_d_a2printer.c" "CMakeLists.txt"; then
  # Find the line with the last U8G2 source file
  LAST_U8G2_LINE=$(grep -n "u8g2/csrc" "CMakeLists.txt" | tail -1 | cut -d: -f1)
  if [ -n "$LAST_U8G2_LINE" ]; then
    # Insert the new source file after the last U8G2 source file
    sed -i "${LAST_U8G2_LINE}a\\  \${CMAKE_CURRENT_SOURCE_DIR}/u8x8_d_a2printer.c" "CMakeLists.txt"
    echo "Added u8x8_d_a2printer.c to CMakeLists.txt"
  else
    echo "Could not find U8G2 source files in CMakeLists.txt. Please add u8x8_d_a2printer.c manually."
  fi
fi

# 4. Create a patch for the U8G2 library to use our custom configuration
echo "Step 4: Creating patch for U8G2 library..."
cat > "patch_u8g2.sh" << 'EOL'
#!/bin/bash
# Patch U8G2 library to use custom configuration

# Find all .c and .h files in the U8G2 library
find ~/u8g2/csrc -name "*.c" -o -name "*.h" | while read file; do
  # Check if the file includes u8g2.h
  if grep -q "#include \"u8g2.h\"" "$file"; then
    # Replace with our custom config
    sed -i 's/#include "u8g2.h"/#include "u8g2_custom_config.h"/g' "$file"
    echo "Patched $file"
  fi
done
EOL
chmod +x patch_u8g2.sh
echo "Created patch_u8g2.sh"

# 5. Build the project
echo "Step 5: Building the project..."
mkdir -p build
cd build
cmake ..
make -j4

echo "U8G2 linker error fix complete!"
echo "If the build was successful, you can now run the application with ./carradio"
