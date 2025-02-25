#!/usr/bin/env python3
"""
This script provides a low-level test of the SSD1306 OLED display
using direct I2C commands without relying on our C++ implementation.
"""

import time
import smbus

# Define constants
OLED_ADDR = 0x3C  # I2C address of SSD1306
OLED_CMD = 0x00   # Command byte
OLED_DATA = 0x40  # Data byte

# Commands
SSD1306_DISPLAYOFF = 0xAE
SSD1306_DISPLAYON = 0xAF
SSD1306_SETDISPLAYCLOCKDIV = 0xD5
SSD1306_SETMULTIPLEX = 0xA8
SSD1306_SETDISPLAYOFFSET = 0xD3
SSD1306_SETSTARTLINE = 0x40
SSD1306_CHARGEPUMP = 0x8D
SSD1306_MEMORYMODE = 0x20
SSD1306_SEGREMAP = 0xA0
SSD1306_COMSCANDEC = 0xC8
SSD1306_SETCOMPINS = 0xDA
SSD1306_SETCONTRAST = 0x81
SSD1306_SETPRECHARGE = 0xD9
SSD1306_SETVCOMDETECT = 0xDB
SSD1306_DISPLAYALLON_RESUME = 0xA4
SSD1306_NORMALDISPLAY = 0xA6
SSD1306_INVERTDISPLAY = 0xA7
SSD1306_COLUMNADDR = 0x21
SSD1306_PAGEADDR = 0x22

def send_command(i2c_bus, cmd):
    """Send command to OLED"""
    try:
        i2c_bus.write_byte_data(OLED_ADDR, OLED_CMD, cmd)
        return True
    except Exception as e:
        print(f"Error sending command 0x{cmd:02X}: {e}")
        return False

def send_data(i2c_bus, data):
    """Send data to OLED"""
    try:
        i2c_bus.write_byte_data(OLED_ADDR, OLED_DATA, data)
        return True
    except Exception as e:
        print(f"Error sending data 0x{data:02X}: {e}")
        return False

def send_data_block(i2c_bus, data_list):
    """Send a block of data to OLED"""
    try:
        for i, data in enumerate(data_list):
            if i % 16 == 0 and i > 0:
                print(f"Sent {i} bytes")
            i2c_bus.write_byte_data(OLED_ADDR, OLED_DATA, data)
        return True
    except Exception as e:
        print(f"Error sending data block at position {i}: {e}")
        return False

def init_display(i2c_bus):
    """Initialize the OLED display"""
    print("Initializing OLED display...")
    
    # Init sequence
    if not send_command(i2c_bus, SSD1306_DISPLAYOFF):
        return False
    if not send_command(i2c_bus, SSD1306_SETDISPLAYCLOCKDIV) or not send_command(i2c_bus, 0x80):
        return False
    if not send_command(i2c_bus, SSD1306_SETMULTIPLEX) or not send_command(i2c_bus, 0x3F):  # 64-1
        return False
    if not send_command(i2c_bus, SSD1306_SETDISPLAYOFFSET) or not send_command(i2c_bus, 0x00):
        return False
    if not send_command(i2c_bus, SSD1306_SETSTARTLINE | 0x00):
        return False
    if not send_command(i2c_bus, SSD1306_CHARGEPUMP) or not send_command(i2c_bus, 0x14):
        return False
    if not send_command(i2c_bus, SSD1306_MEMORYMODE) or not send_command(i2c_bus, 0x00):
        return False
    if not send_command(i2c_bus, SSD1306_SEGREMAP | 0x1):
        return False
    if not send_command(i2c_bus, SSD1306_COMSCANDEC):
        return False
    if not send_command(i2c_bus, SSD1306_SETCOMPINS) or not send_command(i2c_bus, 0x12):
        return False
    if not send_command(i2c_bus, SSD1306_SETCONTRAST) or not send_command(i2c_bus, 0xCF):
        return False
    if not send_command(i2c_bus, SSD1306_SETPRECHARGE) or not send_command(i2c_bus, 0xF1):
        return False
    if not send_command(i2c_bus, SSD1306_SETVCOMDETECT) or not send_command(i2c_bus, 0x40):
        return False
    if not send_command(i2c_bus, SSD1306_DISPLAYALLON_RESUME):
        return False
    if not send_command(i2c_bus, SSD1306_NORMALDISPLAY):
        return False
    if not send_command(i2c_bus, SSD1306_DISPLAYON):
        return False
    
    print("OLED display initialized successfully")
    return True

def clear_display(i2c_bus):
    """Clear the OLED display"""
    print("Clearing display...")
    
    # Set column address from 0 to 127
    send_command(i2c_bus, SSD1306_COLUMNADDR)
    send_command(i2c_bus, 0)
    send_command(i2c_bus, 127)

    # Set page address from 0 to 7 (for 64 pixel height display)
    send_command(i2c_bus, SSD1306_PAGEADDR)
    send_command(i2c_bus, 0)
    send_command(i2c_bus, 7)
    
    # Send 1024 bytes of 0x00 (empty)
    zeros = [0] * 1024
    return send_data_block(i2c_bus, zeros)

def fill_display(i2c_bus):
    """Fill the OLED display with a pattern"""
    print("Filling display with a pattern...")
    
    # Set column address from 0 to 127
    send_command(i2c_bus, SSD1306_COLUMNADDR)
    send_command(i2c_bus, 0)
    send_command(i2c_bus, 127)

    # Set page address from 0 to 7 (for 64 pixel height display)
    send_command(i2c_bus, SSD1306_PAGEADDR)
    send_command(i2c_bus, 0)
    send_command(i2c_bus, 7)
    
    # Create a pattern with alternating lines (checkerboard effect)
    pattern = []
    for page in range(8):  # 8 pages (vertical strips of 8 pixels)
        for column in range(128):  # 128 columns wide
            if (column + page) % 2 == 0:
                pattern.append(0x55)  # 01010101
            else:
                pattern.append(0xAA)  # 10101010
    
    return send_data_block(i2c_bus, pattern)

def draw_text(i2c_bus):
    """Draw a simple text pattern on the OLED"""
    # Simple font - just the letter 'X' (5x7 font)
    X_PATTERN = [
        0b01000010,  # X X
        0b00100100,  #  X X
        0b00011000,  #   X
        0b00100100,  #  X X
        0b01000010,  # X   X
        0b00000000,  #
        0b00000000,  #
    ]
    
    # Set column address from 0 to 127
    send_command(i2c_bus, SSD1306_COLUMNADDR)
    send_command(i2c_bus, 0)
    send_command(i2c_bus, 127)

    # Set page address from 0 to 7 (for 64 pixel height display)
    send_command(i2c_bus, SSD1306_PAGEADDR)
    send_command(i2c_bus, 0)
    send_command(i2c_bus, 7)
    
    # Create a pattern that repeats 'X's across the screen
    pattern = []
    for page in range(8):  # 8 pages (vertical strips of 8 pixels)
        for column in range(128):  # 128 columns wide
            col_in_char = column % 8
            if col_in_char < 5:  # Only the first 5 bits of each column are used in our font
                if page < len(X_PATTERN):
                    pattern.append(X_PATTERN[col_in_char])
                else:
                    pattern.append(0)
            else:
                pattern.append(0)  # Space between characters
    
    return send_data_block(i2c_bus, pattern)

def main():
    print("SSD1306 OLED Low-Level Test")
    try:
        # Open I2C bus
        i2c_bus = smbus.SMBus(1)  # /dev/i2c-1
        print(f"Connected to I2C bus, testing OLED at address 0x{OLED_ADDR:02X}")
        
        # Initialize OLED
        if not init_display(i2c_bus):
            print("Failed to initialize display")
            return
        
        # Clear the display
        if not clear_display(i2c_bus):
            print("Failed to clear display")
            return
        
        print("Display cleared, waiting 1 second...")
        time.sleep(1)
        
        # Fill with pattern
        if not fill_display(i2c_bus):
            print("Failed to fill display")
            return
        
        print("Display filled with pattern, waiting 2 seconds...")
        time.sleep(2)
        
        # Draw text
        if not draw_text(i2c_bus):
            print("Failed to draw text")
            return
        
        print("Text drawn on display")
        print("Test completed successfully")
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
