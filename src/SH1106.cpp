#include "SH1106.hpp"
#include "ErrorMgr.hpp"
#include <algorithm>
#include <cstring>

void SH1106::display() {
    // Debug: Print the first few bytes of the buffer
    if (!_buffer.empty()) {
        printf("SH1106 buffer (first 16 bytes): ");
        for (size_t i = 0; i < std::min(size_t(16), _buffer.size()); i++) {
            printf("%02X ", _buffer[i]);
        }
        printf("\n");
    } else {
        printf("SH1106 buffer is empty!\n");
        return;
    }
    
    // SH1106 uses page addressing mode
    // We need to set page address, then column address for each page
    for (uint8_t page = 0; page < (DISPLAY_HEIGHT / 8); page++) {
        // Set page start address
        sendCommand(SH1106_SETPAGE | page);
        
        // Set column start address with offset (usually 2)
        sendCommand(SH1106_SETLOWCOLUMN | (COLUMN_OFFSET & 0x0F));
        sendCommand(SH1106_SETHIGHCOLUMN | ((COLUMN_OFFSET >> 4) & 0x0F));
        
        // Send data for this page
        for (uint8_t col = 0; col < DISPLAY_WIDTH; col++) {
            uint16_t bufferIndex = page * DISPLAY_WIDTH + col;
            
            if (bufferIndex < _buffer.size()) {
                sendData(_buffer[bufferIndex]);
            } else {
                ELOG_ERROR(ErrorMgr::FAC_I2C, _i2cAddress, 0, 
                          "SH1106 display buffer index out of bounds: %u (size: %zu)",
                          bufferIndex, _buffer.size());
                break;
            }
        }
    }
}

// Override the begin function to provide SH1106-specific initialization
bool SH1106::begin(const char* devicePath) {
    // First call the parent class begin method to initialize I2C
    if (!SSD1306::begin(devicePath)) {
        return false;
    }
    
    // SH1106 specific initialization
    sendCommand(0xAE);  // Display off
    sendCommand(0xA1);  // Segment remap
    sendCommand(0xC8);  // COM scan direction: remapped
    sendCommand(0xA8);  // Set multiplex ratio
    sendCommand(0x3F);  // 64 lines
    sendCommand(0xD3);  // Set display offset
    sendCommand(0x00);  // No offset
    sendCommand(0xD5);  // Set display clock
    sendCommand(0x80);  // Recommended value
    sendCommand(0xD9);  // Set pre-charge period
    sendCommand(0xF1);  // Recommended value for SH1106
    sendCommand(0xDA);  // Set COM pins
    sendCommand(0x12);  // Alternate COM pin config
    sendCommand(0xDB);  // Set VCOMH deselect level
    sendCommand(0x40);  // Default value
    sendCommand(0x81);  // Set contrast control
    sendCommand(0xCF);  // High contrast
    sendCommand(0xA4);  // Resume display
    sendCommand(0x2E);  // Deactivate scroll
    sendCommand(0xAF);  // Display on
    
    printf("SH1106: Display initialized with SH1106-specific settings\n");
    return true;
}
