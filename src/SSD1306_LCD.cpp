#include "SSD1306_LCD.hpp"

// Constants for LCD compatibility
const uint8_t LCD_COLS = 16;
const uint8_t LCD_ROWS = 2;
const uint8_t CHAR_WIDTH = 6;  // Width of each character in pixels
[[maybe_unused]] const uint8_t CHAR_HEIGHT = 8; // Height of each character in pixels
const uint8_t ROW_HEIGHT = 16; // Pixels between rows for better readability (2 * CHAR_HEIGHT)

SSD1306_LCD::SSD1306_LCD(uint8_t i2cAddress) 
    : _oled(i2cAddress), 
      _curRow(0), 
      _curCol(0),
      _displayOn(true) {
}

SSD1306_LCD::~SSD1306_LCD() {
}

bool SSD1306_LCD::begin() {
    if (!_oled.begin())
        return false;
    
    clear();
    return true;
}

void SSD1306_LCD::clear() {
    _oled.clear();
    _oled.display();
    _curRow = 0;
    _curCol = 0;
}

void SSD1306_LCD::setCursor(uint8_t col, uint8_t row) {
    _curCol = (col < LCD_COLS) ? col : LCD_COLS - 1;
    _curRow = (row < LCD_ROWS) ? row : LCD_ROWS - 1;
    
    // Convert LCD character positions to OLED pixel positions
    uint8_t x = _curCol * CHAR_WIDTH;
    uint8_t y = _curRow * ROW_HEIGHT;
    _oled.setCursor(x, y);
}

void SSD1306_LCD::print(const std::string& text) {
    if (!_displayOn) return;  // Don't print if display is off

    // Calculate how many characters we can fit on this line
    int charsLeft = LCD_COLS - _curCol;
    std::string displayText = text.substr(0, charsLeft);
    
    // Print the text
    _oled.print(displayText);
    _oled.display();
    
    // Update cursor position
    _curCol += displayText.length();
    if (_curCol >= LCD_COLS) {
        _curCol = 0;
        _curRow = (_curRow + 1) % LCD_ROWS;
        setCursor(_curCol, _curRow);
    }
}

void SSD1306_LCD::setBacklight(bool on) {
    _displayOn = on;
    if (!on) {
        _oled.clear();
        _oled.display();
    }
}
