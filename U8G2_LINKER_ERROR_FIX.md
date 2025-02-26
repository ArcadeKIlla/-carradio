# U8G2 Linker Error Fix

## The Error

When compiling the CarRadio project with the U8G2 library, we encountered the following linker errors:

```
/usr/bin/ld: CMakeFiles/carradio.dir/home/nate/u8g2/csrc/u8g2_d_setup.c.o: in function `u8g2_Setup_a2printer_384x240_1':
u8g2_d_setup.c:(.text+0x1c4f8): undefined reference to `u8x8_d_a2printer_384x240'
/usr/bin/ld: u8g2_d_setup.c:(.text+0x1c4fc): undefined reference to `u8x8_d_a2printer_384x240'
...
```

## Understanding the Problem

The U8G2 library includes support for many different display types, but not all of them are compiled by default. The error occurs because:

1. The U8G2 library has setup functions for the A2 printer device (`u8g2_Setup_a2printer_384x240_1`, etc.)
2. These setup functions reference the A2 printer device driver function (`u8x8_d_a2printer_384x240`)
3. The actual implementation of the A2 printer device driver is either:
   - Not included in the build
   - Disabled by a configuration macro
   - Missing from the U8G2 library installation

## The Solution

There are three possible approaches to fix this issue:

### 1. Enable the A2 Printer Device Driver

If the A2 printer device driver is disabled by a configuration macro, we can enable it by defining the appropriate macro. The U8G2 library uses macros like `U8X8_WITH_A2PRINTER` to conditionally compile device drivers.

### 2. Provide a Custom Implementation

If the A2 printer device driver is missing, we can provide our own implementation of the `u8x8_d_a2printer_384x240` function.

### 3. Disable the A2 Printer Setup Functions

If we don't need the A2 printer functionality, we can disable the setup functions by modifying the U8G2 library configuration.

## Our Approach

We've created a script (`fix_u8g2_linker_error.sh`) that implements the first two approaches:

1. Creates a custom U8G2 configuration file (`u8g2_custom_config.h`) that enables the A2 printer device driver
2. Provides a custom implementation of the A2 printer device driver (`u8x8_d_a2printer.c`)
3. Updates the CMakeLists.txt to include our custom files
4. Creates a patch script to modify the U8G2 library to use our custom configuration

## How to Apply the Fix

To apply this fix on your Raspberry Pi, follow these steps:

```bash
# SSH into your Raspberry Pi
ssh pi@your-raspberry-pi-ip

# Navigate to your project directory
cd ~/carradio

# Pull the latest changes
git pull

# Make the script executable
chmod +x fix_u8g2_linker_error.sh

# Run the script
./fix_u8g2_linker_error.sh
```

## Technical Details

### Custom Configuration

The `u8g2_custom_config.h` file enables the A2 printer device driver by defining `U8X8_WITH_A2PRINTER`:

```c
#define U8X8_WITH_A2PRINTER 1
```

### Custom Implementation

The `u8x8_d_a2printer.c` file provides a minimal implementation of the A2 printer device driver:

```c
uint8_t u8x8_d_a2printer_384x240(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  // Implementation details...
}
```

### CMake Integration

We update the CMakeLists.txt to:
1. Add the current source directory to the include path
2. Define `U8G2_USE_CUSTOM_CONFIG` to use our custom configuration
3. Add our custom A2 printer implementation to the build

### U8G2 Library Patching

The `patch_u8g2.sh` script modifies the U8G2 library source files to include our custom configuration instead of the default one:

```bash
sed -i 's/#include "u8g2.h"/#include "u8g2_custom_config.h"/g' "$file"
```

## Alternative Approaches

If the above fix doesn't work, you could try these alternatives:

1. **Rebuild the U8G2 library with all device drivers enabled**:
   ```bash
   cd ~/u8g2
   make clean
   make CFLAGS="-DU8X8_WITH_A2PRINTER=1"
   ```

2. **Disable the A2 printer setup functions**:
   Create a custom version of `u8g2_d_setup.c` that doesn't include the A2 printer setup functions.

3. **Use a different display type**:
   If you're not actually using the A2 printer, modify your code to use a different display type that is properly supported.

## Troubleshooting

If you still encounter issues after applying the fix:

1. Check if the A2 printer device driver is actually needed for your project
2. Verify that the custom implementation is being compiled and linked
3. Look for other undefined references in the linker output
4. Consider rebuilding the U8G2 library from source with all device drivers enabled
