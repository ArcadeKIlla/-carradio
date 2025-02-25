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
