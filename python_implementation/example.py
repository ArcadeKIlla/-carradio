#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
example.py
Example of using the VFD_SH1106_Adapter on Raspberry Pi
"""

import time
import sys
from vfd_sh1106_adapter import VFD_SH1106_Adapter

def main():
    """Main function to demonstrate the VFD_SH1106_Adapter"""
    # Create the adapter
    display = VFD_SH1106_Adapter()
    
    # Initialize with I2C on Raspberry Pi
    # /dev/i2c-1 is the default I2C bus on most Raspberry Pi models
    # 0x3C is the default I2C address for most SH1106 displays
    if not display.begin("/dev/i2c-1", 0x3C):
        print("Failed to initialize display!")
        return 1
    
    print("Display initialized successfully.")
    
    try:
        # Clear the screen
        display.clear_screen()
        
        # Set maximum brightness
        display.set_brightness(7)
        
        # Display a simple message
        display.set_cursor(0, 10)
        display.set_font(VFD_SH1106_Adapter.FONT_5x7)
        display.write("CarRadio")
        
        display.set_cursor(0, 25)
        display.set_font(VFD_SH1106_Adapter.FONT_MINI)
        display.write("SH1106 Adapter")
        
        display.set_cursor(0, 40)
        display.write("Running on Raspberry Pi")
        
        # Draw a scroll bar
        display.draw_scroll_bar(0, 0.5, 0.25)
        
        # Wait 2 seconds
        time.sleep(2)
        
        # Display multiple lines
        lines = [
            "Line 1",
            "Line 2",
            "Line 3",
            "Line 4"
        ]
        
        display.clear_screen()
        display.print_lines(10, 10, lines, 0, 4, VFD_SH1106_Adapter.FONT_MINI)
        
        # Wait 2 seconds
        time.sleep(2)
        
        # Display rows with columns
        columns = [
            ["Item 1", "Value 1"],
            ["Item 2", "Value 2"],
            ["Item 3", "Value 3"]
        ]
        
        display.clear_screen()
        display.print_rows(10, 10, columns, 0, 3, 0, VFD_SH1106_Adapter.FONT_MINI)
        
        # Keep the display on for 5 seconds
        time.sleep(5)
        
    except KeyboardInterrupt:
        pass
    finally:
        # Clean up
        display.set_power_on(False)
        display.stop()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
