#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>

#include "u8g2.h"

// Hardware I2C implementation for U8G2 on Linux
uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static int i2c_fd = -1;
    static uint8_t i2c_address;
    
    switch(msg) {
        case U8X8_MSG_BYTE_INIT:
            // Initialize I2C
            if (i2c_fd >= 0) {
                close(i2c_fd);
            }
            
            // Get the I2C device path from u8x8 user_ptr (if available)
            const char* i2c_device = "/dev/i2c-1"; // Default
            if (u8x8->user_ptr != NULL) {
                i2c_device = (const char*)u8x8->user_ptr;
            }
            
            i2c_fd = open(i2c_device, O_RDWR);
            if (i2c_fd < 0) {
                perror("Failed to open I2C device");
                return 0;
            }
            
            // Get the I2C address from u8x8
            i2c_address = u8x8_GetI2CAddress(u8x8) >> 1; // U8G2 uses shifted address
            
            // Set the I2C slave address
            if (ioctl(i2c_fd, I2C_SLAVE, i2c_address) < 0) {
                perror("Failed to set I2C slave address");
                close(i2c_fd);
                i2c_fd = -1;
                return 0;
            }
            
            return 1;
            
        case U8X8_MSG_BYTE_SEND:
            // Send data to I2C device
            if (i2c_fd < 0) {
                return 0;
            }
            
            // Write data to I2C device
            if (write(i2c_fd, arg_ptr, arg_int) != arg_int) {
                perror("Failed to write to I2C device");
                return 0;
            }
            
            return 1;
            
        case U8X8_MSG_BYTE_START_TRANSFER:
            // Nothing to do for I2C start transfer
            return 1;
            
        case U8X8_MSG_BYTE_END_TRANSFER:
            // Nothing to do for I2C end transfer
            return 1;
            
        case U8X8_MSG_BYTE_SET_DC:
            // Not used for I2C
            return 1;
            
        default:
            return 0;
    }
}
