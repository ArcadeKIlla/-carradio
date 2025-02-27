#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
VFD_SH1106_Adapter.py
CarRadio SH1106 Display Adapter (Python Version)

This adapter converts VFD commands to SH1106 OLED display commands using the luma.oled library
Optimized for Raspberry Pi deployment
"""

import time
import logging
from PIL import Image, ImageDraw, ImageFont
from luma.core.interface.serial import i2c
from luma.oled.device import sh1106

class VFD_SH1106_Adapter:
    """
    This class implements a similar interface to the original VFD class
    but uses the luma.oled library to drive an SH1106 OLED display
    """
    
    # Constants from original VFD class
    VFD_OUTLINE = 0x14
    VFD_CLEAR_AREA = 0x12
    VFD_SET_AREA = 0x11
    VFD_SET_CURSOR = 0x10
    VFD_SET_WRITEMODE = 0x1A
    
    SCROLL_BAR_WIDTH = 3
    
    # Font types
    FONT_MINI = 0
    FONT_5x7 = 1
    FONT_10x14 = 2
    
    def __init__(self):
        """Initialize the adapter"""
        self._is_setup = False
        self._cursor_x = 0
        self._cursor_y = 0
        self._current_font = self.FONT_MINI
        self._i2c_address = 0x3C  # Default I2C address for most SH1106 displays
        self._device = None
        self._image = None
        self._draw = None
        self._font = None
        self._font_size = 8  # Default font size
        
    def begin(self, path, address=0x3C):
        """
        Initialize the display with I2C
        
        Args:
            path (str): I2C device path (e.g., "/dev/i2c-1")
            address (int): I2C device address (default: 0x3C)
            
        Returns:
            bool: True if initialization was successful, False otherwise
        """
        try:
            # Extract I2C bus number from path
            bus_num = int(path.split('-')[-1])
            
            # Initialize I2C interface
            serial_interface = i2c(port=bus_num, address=address)
            
            # Initialize SH1106 device
            self._device = sh1106(serial_interface, width=128, height=64, rotate=0)
            
            # Create image buffer
            self._image = Image.new('1', (self._device.width, self._device.height))
            self._draw = ImageDraw.Draw(self._image)
            
            # Set default font
            self.set_font(self.FONT_MINI)
            
            # Clear the display
            self.clear_screen()
            
            # Set full brightness
            self.set_brightness(7)
            
            self._is_setup = True
            return True
        except Exception as e:
            logging.error(f"Error initializing display: {e}")
            return False
    
    def stop(self):
        """Clean up resources"""
        if self._device:
            self._device.cleanup()
            self._device = None
        self._is_setup = False
    
    def reset(self):
        """Reset the display"""
        if not self._is_setup:
            return False
        
        self._device.clear()
        self._image = Image.new('1', (self._device.width, self._device.height))
        self._draw = ImageDraw.Draw(self._image)
        self._cursor_x = 0
        self._cursor_y = 0
        return True
    
    def set_brightness(self, level):
        """
        Set the display brightness
        
        Args:
            level (int): Brightness level (0-7), where 0 is off and 7 is maximum
            
        Returns:
            bool: True if successful, False otherwise
        """
        if not self._is_setup:
            return False
        
        # Convert 0-7 scale to 0-255
        brightness = min(255, int(level * 255 / 7))
        self._device.contrast(brightness)
        return True
    
    def set_power_on(self, set_on):
        """
        Turn the display on or off
        
        Args:
            set_on (bool): True to turn on, False to turn off
            
        Returns:
            bool: True if successful, False otherwise
        """
        if not self._is_setup:
            return False
        
        if set_on:
            self._device.show()
        else:
            self._device.hide()
        return True
    
    def set_cursor(self, x, y):
        """
        Set the cursor position
        
        Args:
            x (int): X coordinate
            y (int): Y coordinate
            
        Returns:
            bool: True if successful, False otherwise
        """
        if not self._is_setup:
            return False
        
        self._cursor_x = x
        self._cursor_y = y
        return True
    
    def set_font(self, font):
        """
        Set the font
        
        Args:
            font (int): Font type (FONT_MINI, FONT_5x7, or FONT_10x14)
            
        Returns:
            bool: True if successful, False otherwise
        """
        if not self._is_setup:
            return False
        
        self._current_font = font
        
        # Update font based on selected type
        try:
            if font == self.FONT_MINI:
                self._font_size = 8
                self._font = ImageFont.truetype("DejaVuSansMono.ttf", self._font_size)
            elif font == self.FONT_5x7:
                self._font_size = 12
                self._font = ImageFont.truetype("DejaVuSansMono.ttf", self._font_size)
            elif font == self.FONT_10x14:
                self._font_size = 16
                self._font = ImageFont.truetype("DejaVuSansMono.ttf", self._font_size)
            else:
                # Default to mini font
                self._font_size = 8
                self._font = ImageFont.truetype("DejaVuSansMono.ttf", self._font_size)
        except IOError:
            # If font file not found, use default bitmap font
            if font == self.FONT_MINI:
                self._font_size = 8
                self._font = ImageFont.load_default()
            elif font == self.FONT_5x7:
                self._font_size = 12
                self._font = ImageFont.load_default()
            elif font == self.FONT_10x14:
                self._font_size = 16
                self._font = ImageFont.load_default()
            else:
                # Default to mini font
                self._font_size = 8
                self._font = ImageFont.load_default()
                
        return True
    
    def clear_screen(self):
        """
        Clear the screen
        
        Returns:
            bool: True if successful, False otherwise
        """
        if not self._is_setup:
            return False
        
        self._draw.rectangle((0, 0, self._device.width-1, self._device.height-1), fill=0)
        self._device.display(self._image)
        return True
    
    def draw_scroll_bar(self, top, bar_height, starting_offset):
        """
        Draw a scroll bar
        
        Args:
            top (int): Top position of the scroll bar
            bar_height (float): Height of the scroll bar as a proportion (0.0-1.0)
            starting_offset (float): Starting offset as a proportion (0.0-1.0)
            
        Returns:
            None
        """
        if not self._is_setup:
            return
        
        # Calculate scroll bar dimensions
        display_height = self._device.height
        scroll_bar_x = self._device.width - self.SCROLL_BAR_WIDTH
        
        # Draw scroll bar background
        self._draw.rectangle(
            (scroll_bar_x, 0, self._device.width-1, display_height-1),
            fill=0
        )
        
        # Calculate scroll bar position and size
        bar_top = int(starting_offset * display_height)
        bar_height_px = max(4, int(bar_height * display_height))
        
        # Draw scroll bar
        self._draw.rectangle(
            (scroll_bar_x, bar_top, self._device.width-1, bar_top + bar_height_px - 1),
            fill=1
        )
        
        # Update display
        self._device.display(self._image)
    
    def write(self, text):
        """
        Write text at the current cursor position
        
        Args:
            text (str): Text to write
            
        Returns:
            bool: True if successful, False otherwise
        """
        if not self._is_setup:
            return False
        
        self._draw.text((self._cursor_x, self._cursor_y), text, font=self._font, fill=1)
        self._device.display(self._image)
        return True
    
    def write_packet(self, data, length, wait_usec=50):
        """
        Write a packet of data
        
        Args:
            data (bytes): Data to write
            length (int): Length of data
            wait_usec (int): Wait time in microseconds (not used in this implementation)
            
        Returns:
            bool: True if successful, False otherwise
        """
        if not self._is_setup or length == 0:
            return False
        
        # Check if this is a command
        if data[0] == self.VFD_SET_CURSOR and length >= 3:
            # Set cursor command
            x = data[1]
            y = data[2]
            self.set_cursor(x, y)
        elif data[0] == self.VFD_CLEAR_AREA and length >= 5:
            # Clear area command
            x1 = data[1]
            y1 = data[2]
            x2 = data[3]
            y2 = data[4]
            self._draw.rectangle((x1, y1, x2, y2), fill=0)
            self._device.display(self._image)
        else:
            # Treat as text data
            try:
                text = data[0:length].decode('utf-8')
                self.write(text)
            except UnicodeDecodeError:
                # If not valid UTF-8, just write each byte as a character
                for i in range(length):
                    self.write(chr(data[i]))
        
        return True
    
    def print_packet(self, fmt, *args):
        """
        Print a formatted packet
        
        Args:
            fmt (str): Format string
            *args: Arguments for the format string
            
        Returns:
            bool: True if successful, False otherwise
        """
        if not self._is_setup:
            return False
        
        text = fmt % args
        return self.write(text)
    
    def print_lines(self, y, step, lines, first_line, max_lines, font=FONT_MINI):
        """
        Print multiple lines of text
        
        Args:
            y (int): Starting Y position
            step (int): Vertical step between lines
            lines (list): List of strings to print
            first_line (int): Index of the first line to print
            max_lines (int): Maximum number of lines to print
            font (int): Font to use
            
        Returns:
            bool: True if successful, False otherwise
        """
        if not self._is_setup:
            return False
        
        # Set the font
        self.set_font(font)
        
        # Calculate how many lines we can display
        num_lines = len(lines)
        display_lines = min(max_lines, num_lines - first_line)
        
        if display_lines <= 0:
            return True  # Nothing to display
        
        # Display each line
        for i in range(display_lines):
            line_idx = first_line + i
            if line_idx < num_lines:
                self.set_cursor(0, y + i * step)
                self.write(lines[line_idx])
        
        return True
    
    def print_rows(self, y, step, columns, first_line, max_lines, x_offset=0, font=FONT_MINI):
        """
        Print rows with multiple columns
        
        Args:
            y (int): Starting Y position
            step (int): Vertical step between rows
            columns (list): List of lists, where each inner list contains strings for each column
            first_line (int): Index of the first row to print
            max_lines (int): Maximum number of rows to print
            x_offset (int): Horizontal offset for the first column
            font (int): Font to use
            
        Returns:
            bool: True if successful, False otherwise
        """
        if not self._is_setup:
            return False
        
        # Set the font
        self.set_font(font)
        
        # Calculate how many rows we can display
        num_rows = len(columns)
        display_rows = min(max_lines, num_rows - first_line)
        
        if display_rows <= 0:
            return True  # Nothing to display
        
        # Calculate column widths
        col_widths = []
        max_cols = 0
        
        for row in columns:
            max_cols = max(max_cols, len(row))
        
        if max_cols == 0:
            return True  # No columns to display
        
        # Find the maximum width for each column
        for col_idx in range(max_cols):
            max_width = 0
            for row in columns:
                if col_idx < len(row):
                    # Estimate text width based on character count and font size
                    # This is a rough approximation
                    text_width = len(row[col_idx]) * (self._font_size // 2)
                    max_width = max(max_width, text_width)
            col_widths.append(max_width + 5)  # Add some padding
        
        # Display each row
        for i in range(display_rows):
            row_idx = first_line + i
            if row_idx < num_rows:
                row = columns[row_idx]
                y_pos = y + i * step
                
                # Display each column in the row
                x_pos = x_offset
                for col_idx, col_text in enumerate(row):
                    self.set_cursor(x_pos, y_pos)
                    self.write(col_text)
                    
                    # Move to the next column position
                    if col_idx < len(col_widths):
                        x_pos += col_widths[col_idx]
        
        return True
    
    def width(self):
        """
        Get the display width
        
        Returns:
            int: Display width in pixels
        """
        return 128 if self._device else 0
    
    def height(self):
        """
        Get the display height
        
        Returns:
            int: Display height in pixels
        """
        return 64 if self._device else 0
    
    @staticmethod
    def truncate(text, width):
        """
        Truncate text to fit within a specified width
        
        Args:
            text (str): Text to truncate
            width (int): Maximum width
            
        Returns:
            str: Truncated text
        """
        if len(text) <= width:
            return text
        return text[:width-3] + "..."
