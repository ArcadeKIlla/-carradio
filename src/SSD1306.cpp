#include "SSD1306.hpp"
#include "ErrorMgr.hpp"
#include <algorithm>
#include <cstring>
#include <unistd.h>

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

// Basic 5x7 font for ASCII characters 32-127
static const uint8_t font[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // Space (32)
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x08, 0x2A, 0x1C, 0x2A, 0x08, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x41, 0x22, 0x14, 0x08, 0x00, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x01, 0x01, // F
    0x3E, 0x41, 0x41, 0x49, 0x3A, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x04, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x7F, 0x20, 0x18, 0x20, 0x7F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x03, 0x04, 0x78, 0x04, 0x03, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x00, 0x7F, 0x41, 0x41, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // "\"
    0x41, 0x41, 0x7F, 0x00, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x08, 0x14, 0x54, 0x54, 0x3C, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, // j
    0x00, 0x7F, 0x10, 0x28, 0x44, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x08, 0x36, 0x41, 0x00, // {
    0x00, 0x00, 0x7F, 0x00, 0x00, // |
    0x00, 0x41, 0x36, 0x08, 0x00, // }
    0x08, 0x08, 0x2A, 0x1C, 0x08, // ->
    0x08, 0x1C, 0x2A, 0x08, 0x08  // <-
};

#define FONT_WIDTH 5
#define FONT_HEIGHT 7
#define FIRST_CHAR 32
#define LAST_CHAR 127

SSD1306::SSD1306(uint8_t i2cAddress) 
    : _i2cAddress(i2cAddress), 
      _buffer(DISPLAY_WIDTH * DISPLAY_HEIGHT / 8, 0),
      _cursorX(0), 
      _cursorY(0),
      _textSize(1),
      _inverted(false) {
}

SSD1306::~SSD1306() {
}

bool SSD1306::begin(const char* path) {
    if (!_i2c.begin(_i2cAddress, path, errno)) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 I2C begin failed");
        return false;
    }

    // Make sure buffer is properly initialized
    if (_buffer.size() != (DISPLAY_WIDTH * DISPLAY_HEIGHT / 8)) {
        _buffer.resize(DISPLAY_WIDTH * DISPLAY_HEIGHT / 8, 0);
        printf("SSD1306: Buffer resized to %zu bytes\n", _buffer.size());
    } else {
        std::fill(_buffer.begin(), _buffer.end(), 0);
        printf("SSD1306: Buffer cleared, size: %zu bytes\n", _buffer.size());
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

    // Debug: Print the first few bytes of the buffer
    if (!_buffer.empty()) {
        printf("SSD1306 buffer (first 16 bytes): ");
        for (size_t i = 0; i < std::min(size_t(16), _buffer.size()); i++) {
            printf("%02X ", _buffer[i]);
        }
        printf("\n");
    } else {
        printf("SSD1306 buffer is empty!\n");
    }

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

void SSD1306::drawChar(int16_t x, int16_t y, char c, bool white) {
    if (c < FIRST_CHAR || c > LAST_CHAR)
        c = '?'; // Replace non-printable characters with '?'
    
    // Get the character index in the font array
    uint8_t charIndex = c - FIRST_CHAR;
    
    // Draw each column of the character
    for (uint8_t col = 0; col < FONT_WIDTH; col++) {
        uint8_t line = font[charIndex * FONT_WIDTH + col];
        for (uint8_t row = 0; row < FONT_HEIGHT; row++) {
            if (line & (1 << row)) {
                // Scale based on text size
                for (uint8_t i = 0; i < _textSize; i++) {
                    for (uint8_t j = 0; j < _textSize; j++) {
                        drawPixel(x + col * _textSize + i, y + row * _textSize + j, white);
                    }
                }
            }
        }
    }
}

void SSD1306::print(const std::string& text) {
    for (char c : text) {
        // Draw character
        drawChar(_cursorX, _cursorY, c, true);
        
        // Move cursor
        _cursorX += (FONT_WIDTH + 1) * _textSize;
        if (_cursorX > DISPLAY_WIDTH - (FONT_WIDTH * _textSize)) {
            _cursorX = 0;
            _cursorY += (FONT_HEIGHT + 1) * _textSize;
            if (_cursorY > DISPLAY_HEIGHT - (FONT_HEIGHT * _textSize)) {
                _cursorY = 0; // Wrap to top
            }
        }
    }
}

void SSD1306::drawPixel(int16_t x, int16_t y, bool white) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT)
        return;

    // Each byte in the buffer represents a vertical column of 8 pixels
    // The SSD1306 memory is arranged in horizontal strips of 8 pixels high
    uint16_t byteIdx = (y / 8) * DISPLAY_WIDTH + x;
    uint8_t bitIdx = y % 8;
    
    if (byteIdx >= _buffer.size()) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, 
                  "SSD1306 drawPixel: Invalid buffer index: %u (size: %zu)", 
                  byteIdx, _buffer.size());
        return;
    }
    
    if (white) {
        _buffer[byteIdx] |= (1 << bitIdx);
    } else {
        _buffer[byteIdx] &= ~(1 << bitIdx);
    }
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
    if (::write(_i2c._fd, buffer, 2) != 2) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, "SSD1306 command 0x%02X failed", command);
        return false;
    }
    return true;
}

void SSD1306::sendData(uint8_t data) {
    uint8_t buffer[2] = {0x40, data};  // Control byte 0x40 for data
    ssize_t bytesWritten = ::write(_i2c._fd, buffer, 2);
    if (bytesWritten != 2) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, 
                  "SSD1306 single data byte write failed: wrote %zd of 2 bytes", 
                  bytesWritten);
    }
}

void SSD1306::sendData(const std::vector<uint8_t>& buffer) {
    // For bulk data, we need to send the control byte followed by data bytes
    std::vector<uint8_t> txBuffer;
    txBuffer.reserve(buffer.size() + 1);
    txBuffer.push_back(0x40);  // Control byte for data
    txBuffer.insert(txBuffer.end(), buffer.begin(), buffer.end());
    
    ssize_t bytesWritten = ::write(_i2c._fd, txBuffer.data(), txBuffer.size());
    if (bytesWritten != static_cast<ssize_t>(txBuffer.size())) {
        ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, errno, 
                  "SSD1306 data write failed: wrote %zd of %zu bytes", 
                  bytesWritten, txBuffer.size());
    }
}
