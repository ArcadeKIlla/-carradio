#include "SSD1306.hpp"
#include "ErrorMgr.hpp"
#include <algorithm>
#include <cstring>

// SSD1306 Commands
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB

// Font data will be implemented when text rendering is needed
// For now, we're only using basic graphics primitives

SSD1306::SSD1306(uint8_t i2cAddress) 
    : _i2cAddress(i2cAddress), 
      _buffer(DISPLAY_WIDTH * DISPLAY_HEIGHT / 8, 0),
      _cursorX(0), 
      _cursorY(0),
      _textSize(1) {
}

SSD1306::~SSD1306() {
}

bool SSD1306::begin(const char* path) {
    if (!_i2c.begin(_i2cAddress, path, errno)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 I2C begin failed");
        return false;
    }

    // Init sequence with error logging
    if (!sendCommand(SSD1306_DISPLAYOFF)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Display Off failed");
        return false;
    }
    if (!sendCommand(SSD1306_SETDISPLAYCLOCKDIV) || !sendCommand(0x80)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Clock Div failed");
        return false;
    }
    if (!sendCommand(SSD1306_SETMULTIPLEX) || !sendCommand(DISPLAY_HEIGHT - 1)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Multiplex failed");
        return false;
    }
    if (!sendCommand(SSD1306_SETDISPLAYOFFSET) || !sendCommand(0x00)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Display Offset failed");
        return false;
    }
    if (!sendCommand(SSD1306_SETSTARTLINE | 0x00)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Start Line failed");
        return false;
    }
    if (!sendCommand(SSD1306_CHARGEPUMP) || !sendCommand(0x14)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Charge Pump failed");
        return false;
    }
    if (!sendCommand(SSD1306_MEMORYMODE) || !sendCommand(0x00)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Memory Mode failed");
        return false;
    }
    if (!sendCommand(SSD1306_SEGREMAP | 0x1)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Segment Remap failed");
        return false;
    }
    if (!sendCommand(SSD1306_COMSCANDEC)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 COM Scan Dec failed");
        return false;
    }
    if (!sendCommand(SSD1306_SETCOMPINS) || !sendCommand(0x12)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 COM Pins failed");
        return false;
    }
    if (!sendCommand(SSD1306_SETCONTRAST) || !sendCommand(0xCF)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Contrast failed");
        return false;
    }
    if (!sendCommand(SSD1306_SETPRECHARGE) || !sendCommand(0xF1)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Precharge failed");
        return false;
    }
    if (!sendCommand(SSD1306_SETVCOMDETECT) || !sendCommand(0x40)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 VCOM Detect failed");
        return false;
    }
    if (!sendCommand(SSD1306_DISPLAYALLON_RESUME)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Display All On Resume failed");
        return false;
    }
    if (!sendCommand(SSD1306_NORMALDISPLAY)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Normal Display failed");
        return false;
    }
    if (!sendCommand(SSD1306_DISPLAYON)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 Display On failed");
        return false;
    }

    clear();
    display();
    return true;
}

void SSD1306::clear() {
    std::fill(_buffer.begin(), _buffer.end(), 0);
}

void SSD1306::display() {
    sendCommand(SSD1306_COLUMNADDR);
    sendCommand(0);
    sendCommand(DISPLAY_WIDTH - 1);
    sendCommand(SSD1306_PAGEADDR);
    sendCommand(0);
    sendCommand((DISPLAY_HEIGHT / 8) - 1);

    sendData(_buffer);
}

void SSD1306::setContrast(uint8_t contrast) {
    sendCommand(SSD1306_SETCONTRAST);
    sendCommand(contrast);
}

void SSD1306::invertDisplay(bool invert) {
    sendCommand(invert ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
}

void SSD1306::setCursor(uint8_t x, uint8_t y) {
    _cursorX = x;
    _cursorY = y;
}

void SSD1306::setTextSize(uint8_t size) {
    _textSize = size;
}

void SSD1306::print(const std::string& text) {
    // Simple implementation - just draw pixels for each character
    // This should be enhanced with proper font rendering
    for (char c : text) {
        // Skip non-printable characters
        if (c < 32 || c > 127)
            continue;

        // Draw character (placeholder implementation)
        // TODO: Implement proper font rendering
        drawRect(_cursorX, _cursorY, 6, 8, false, true);
        
        // Move cursor
        _cursorX += 6 * _textSize;
        if (_cursorX > DISPLAY_WIDTH - 6) {
            _cursorX = 0;
            _cursorY += 8 * _textSize;
        }
    }
}

void SSD1306::drawPixel(int16_t x, int16_t y, bool white) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT)
        return;

    int16_t byteIndex = x + (y / 8) * DISPLAY_WIDTH;
    if (white)
        _buffer[byteIndex] |= (1 << (y & 7));
    else
        _buffer[byteIndex] &= ~(1 << (y & 7));
}

void SSD1306::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool white) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }

    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++) {
        if (steep)
            drawPixel(y0, x0, white);
        else
            drawPixel(x0, y0, white);
        
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void SSD1306::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool fill, bool white) {
    if (fill) {
        for (int16_t i = x; i < x + w; i++) {
            for (int16_t j = y; j < y + h; j++) {
                drawPixel(i, j, white);
            }
        }
    }
    else {
        drawLine(x, y, x + w - 1, y, white);
        drawLine(x + w - 1, y, x + w - 1, y + h - 1, white);
        drawLine(x + w - 1, y + h - 1, x, y + h - 1, white);
        drawLine(x, y + h - 1, x, y, white);
    }
}

bool SSD1306::sendCommand(uint8_t command) {
    uint8_t buffer[2] = {0x00, command};  // Control byte 0x00 for command
    if (write(_i2c._fd, buffer, 2) != 2) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 command 0x%02X failed", command);
        return false;
    }
    return true;
}

void SSD1306::sendData(uint8_t data) {
    uint8_t buffer[2] = {0x40, data};  // Control byte 0x40 for data
    write(_i2c._fd, buffer, 2);
}

void SSD1306::sendData(const std::vector<uint8_t>& buffer) {
    // For bulk data, we need to send the control byte followed by data bytes
    std::vector<uint8_t> txBuffer;
    txBuffer.reserve(buffer.size() + 1);
    txBuffer.push_back(0x40);  // Control byte for data
    txBuffer.insert(txBuffer.end(), buffer.begin(), buffer.end());
    write(_i2c._fd, txBuffer.data(), txBuffer.size());
}
