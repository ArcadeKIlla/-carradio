//
//  VFD_SH1106_Adapter.cpp
//  carradio_v2_sh1106
//
//  Created on 2/26/2025
//  This adapter converts VFD commands to SH1106 OLED display commands using the u8g2 library
//  Optimized for Raspberry Pi deployment
//

#include "VFD_SH1106_Adapter.hpp"
#include <stdarg.h>

VFD_SH1106_Adapter::VFD_SH1106_Adapter() {
    _isSetup = false;
    _cursorX = 0;
    _cursorY = 0;
    _currentFont = FONT_MINI;
    _u8g2 = nullptr;
    _i2cAddress = 0x3C; // Default I2C address for most SH1106 displays
}

VFD_SH1106_Adapter::~VFD_SH1106_Adapter() {
    stop();
}

bool VFD_SH1106_Adapter::begin(const char* path, speed_t speed) {
    int error = 0;
    return begin(path, speed, error);
}

bool VFD_SH1106_Adapter::begin(const char* path, speed_t speed, int &error) {
    // For compatibility with the original VFD class
    // On Raspberry Pi, we'll use the I2C address version instead
    return begin(path, _i2cAddress);
}

bool VFD_SH1106_Adapter::begin(const char* path, uint8_t i2cAddress) {
    // Raspberry Pi specific initialization
    _i2cAddress = i2cAddress;
    
    // Create the U8G2 object for SH1106 with I2C
    // U8G2_R0 means no rotation, display is drawn normal
    _u8g2 = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0);
    
    if (_u8g2 == nullptr) {
        return false;
    }
    
    // Set I2C address if different from default
    if (_i2cAddress != 0x3C) {
        _u8g2->setI2CAddress(_i2cAddress * 2); // U8g2 requires the address to be shifted
    }
    
    // Initialize the display
    _u8g2->begin();
    _u8g2->clearBuffer();
    _u8g2->sendBuffer();
    
    // Set default font
    setFont(FONT_MINI);
    
    _isSetup = true;
    return _isSetup;
}

void VFD_SH1106_Adapter::stop() {
    if (_isSetup && _u8g2 != nullptr) {
        delete _u8g2;
        _u8g2 = nullptr;
    }
    _isSetup = false;
}

bool VFD_SH1106_Adapter::reset() {
    if (!_isSetup || _u8g2 == nullptr)
        return false;
    
    _u8g2->initDisplay();
    _u8g2->clearBuffer();
    _u8g2->sendBuffer();
    return true;
}

bool VFD_SH1106_Adapter::setBrightness(uint8_t level) {
    if (!_isSetup || _u8g2 == nullptr)
        return false;
    
    // Map 0-7 to 0-255
    uint8_t contrast = (level * 255) / 7;
    _u8g2->setContrast(contrast);
    return true;
}

bool VFD_SH1106_Adapter::setPowerOn(bool setOn) {
    if (!_isSetup || _u8g2 == nullptr)
        return false;
    
    _u8g2->setPowerSave(!setOn);
    return true;
}

bool VFD_SH1106_Adapter::setCursor(uint8_t x, uint8_t y) {
    if (!_isSetup || _u8g2 == nullptr)
        return false;
    
    _cursorX = x;
    _cursorY = y;
    return true;
}

bool VFD_SH1106_Adapter::setFont(font_t font) {
    if (!_isSetup || _u8g2 == nullptr)
        return false;
    
    _currentFont = font;
    updateFont();
    return true;
}

void VFD_SH1106_Adapter::updateFont() {
    switch (_currentFont) {
        case FONT_MINI:
            _u8g2->setFont(u8g2_font_5x8_tr); // Small font
            break;
        case FONT_5x7:
            _u8g2->setFont(u8g2_font_6x10_tr); // Medium font
            break;
        case FONT_10x14:
            _u8g2->setFont(u8g2_font_10x20_tr); // Large font
            break;
    }
}

bool VFD_SH1106_Adapter::clearScreen() {
    if (!_isSetup || _u8g2 == nullptr)
        return false;
    
    _u8g2->clearBuffer();
    _u8g2->sendBuffer();
    return true;
}

void VFD_SH1106_Adapter::drawScrollBar(uint8_t topbox, float bar_height, float starting_offset) {
    if (!_isSetup || _u8g2 == nullptr)
        return;
    
    uint8_t rightbox = width() - 1;
    uint8_t leftbox = rightbox - scroll_bar_width + 1;
    uint8_t bottombox = 63;
    uint8_t scroll_height = bottombox - topbox - 2;
    uint8_t bar_size = ceil(scroll_height * bar_height);
    uint8_t offset = ((scroll_height - bar_size) * starting_offset) + topbox + 1;
    
    // Draw outline
    _u8g2->drawFrame(leftbox, topbox, scroll_bar_width, bottombox - topbox + 1);
    
    // Draw the scroll indicator
    _u8g2->drawBox(leftbox + 1, offset, scroll_bar_width - 2, bar_size);
    
    sendBuffer();
}

bool VFD_SH1106_Adapter::write(const char* str) {
    if (!_isSetup || _u8g2 == nullptr)
        return false;
    
    _u8g2->setCursor(_cursorX, _cursorY);
    _u8g2->drawStr(_cursorX, _cursorY, str);
    sendBuffer();
    return true;
}

bool VFD_SH1106_Adapter::write(string str) {
    return write(str.c_str());
}

bool VFD_SH1106_Adapter::writePacket(const uint8_t *data, size_t len, useconds_t waitusec) {
    // This is a simplified implementation that doesn't handle all the special commands
    // from the original VFD class. For complex commands, we'd need to parse and interpret them.
    
    if (!_isSetup || _u8g2 == nullptr || len == 0)
        return false;
    
    // Check for special commands
    if (len > 0) {
        switch (data[0]) {
            case VFD_OUTLINE:
                if (len >= 5) {
                    // Draw a frame
                    _u8g2->drawFrame(data[1], data[2], data[3] - data[1] + 1, data[4] - data[2] + 1);
                    sendBuffer();
                    return true;
                }
                break;
                
            case VFD_CLEAR_AREA:
                if (len >= 5) {
                    // Clear an area by drawing a filled box with background color
                    _u8g2->setDrawColor(0);
                    _u8g2->drawBox(data[1], data[2], data[3] - data[1] + 1, data[4] - data[2] + 1);
                    _u8g2->setDrawColor(1);
                    sendBuffer();
                    return true;
                }
                break;
                
            case VFD_SET_AREA:
                if (len >= 5) {
                    // Fill an area
                    _u8g2->drawBox(data[1], data[2], data[3] - data[1] + 1, data[4] - data[2] + 1);
                    sendBuffer();
                    return true;
                }
                break;
                
            case VFD_SET_CURSOR:
                if (len >= 3) {
                    setCursor(data[1], data[2]);
                    return true;
                }
                break;
                
            // Handle other special commands as needed
        }
    }
    
    // If not a special command, treat as text
    char* text = new char[len + 1];
    memcpy(text, data, len);
    text[len] = '\0';
    
    bool result = write(text);
    delete[] text;
    
    return result;
}

bool VFD_SH1106_Adapter::printPacket(const char *fmt, ...) {
    if (!_isSetup || _u8g2 == nullptr)
        return false;
    
    char* s;
    va_list args;
    va_start(args, fmt);
    vasprintf(&s, fmt, args);
    
    bool success = write(s);
    free(s);
    va_end(args);
    
    return success;
}

bool VFD_SH1106_Adapter::printLines(uint8_t y, uint8_t step, vector<string> lines,
                                    uint8_t firstLine, uint8_t maxLines, 
                                    font_t font) {
    if (!_isSetup || _u8g2 == nullptr)
        return false;
    
    auto lineCount = lines.size();
    
    setFont(font);
    
    if (maxLines >= lineCount) {
        // Ignore the offset and draw all lines
        for (int i = 0; i < lineCount; i++) {
            string str = lines[i];
            if (!str.empty()) {
                setCursor(0, y);
                bool success = printPacket("%s", str.c_str());
                if (!success) return false;
            }
            y += step;
        }
    } else {
        // This text needs to be scrolled
        auto maxFirstLine = lineCount - maxLines;
        if (firstLine > maxFirstLine) firstLine = maxFirstLine;
        
        auto count = lineCount - firstLine;
        if (count > maxLines) count = maxLines;
        
        for (auto i = firstLine; i < firstLine + count; i++) {
            string str = lines[i];
            
            // Calculate available width considering scroll bar
            uint8_t rightbox = width() - 1 - scroll_bar_width;
            
            // Clear the line area
            _u8g2->setDrawColor(0);
            _u8g2->drawBox(0, y - step + 1, rightbox, step - 1);
            _u8g2->setDrawColor(1);
            
            setCursor(0, y);
            bool success = printPacket("%s", str.c_str());
            if (!success) return false;
            
            y += step;
        }
    }
    
    return true;
}

bool VFD_SH1106_Adapter::printRows(uint8_t y, uint8_t step, vector<vector<string>> columns,
                                   uint8_t firstLine, uint8_t maxLines, uint8_t col1_start,
                                   font_t font) {
    if (!_isSetup || _u8g2 == nullptr)
        return false;
    
    auto lineCount = columns.size();
    
    setFont(font);
    
    // Calculate column widths
    uint8_t longest_col1_pixel_width = 0;
    
    for (auto &row : columns) {
        if (row.size() > 0 && !row[0].empty()) {
            // Estimate width based on character count and font
            int charWidth;
            switch (font) {
                case FONT_MINI: charWidth = 5; break;
                case FONT_5x7: charWidth = 6; break;
                case FONT_10x14: charWidth = 10; break;
                default: charWidth = 5;
            }
            
            uint8_t length = row[0].length() * charWidth;
            if (length > longest_col1_pixel_width)
                longest_col1_pixel_width = length;
        }
    }
    
    uint8_t col2_start = col1_start + longest_col1_pixel_width + 2;
    
    if (maxLines >= lineCount) {
        // Ignore the offset and draw all
        for (int i = 0; i < lineCount; i++) {
            vector<string> row = columns[i];
            string str = row[0];
            string col2 = "";
            if (row.size() > 1) col2 = row[1];
            
            setCursor(col1_start, y);
            bool success = printPacket("%s", str.c_str());
            if (!success) return false;
            
            setCursor(col2_start, y);
            success = printPacket("%s", col2.c_str());
            if (!success) return false;
            
            y += step;
        }
    } else {
        // This text needs to be scrolled
        auto maxFirstLine = lineCount - maxLines;
        if (firstLine > maxFirstLine) firstLine = maxFirstLine;
        
        auto count = lineCount - firstLine;
        if (count > maxLines) count = maxLines;
        
        for (auto i = firstLine; i < firstLine + count; i++) {
            vector<string> row = columns[i];
            string str = row[0];
            string col2 = "";
            if (row.size() > 1) col2 = row[1];
            
            // Clear the line area
            _u8g2->setDrawColor(0);
            _u8g2->drawBox(col1_start, y - step + 1, width() - col1_start - scroll_bar_width, step - 1);
            _u8g2->setDrawColor(1);
            
            setCursor(col1_start, y);
            bool success = printPacket("%s", str.c_str());
            
            setCursor(col2_start, y);
            if (success && !col2.empty()) {
                success = printPacket("%s", col2.c_str());
            }
            
            if (!success) return false;
            
            y += step;
        }
    }
    
    return true;
}

void VFD_SH1106_Adapter::sendBuffer() {
    if (_isSetup && _u8g2 != nullptr) {
        _u8g2->sendBuffer();
    }
}
