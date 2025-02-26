#include "u8g2.h"
#include "I2C.hpp"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <map>

// Map to store I2C file descriptors for each address
static std::map<uint8_t, int> i2c_fds;

// Hardware I2C implementation for u8g2
uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static int fd = -1;
    static uint8_t device_address;
    
    switch(msg)
    {
        case U8X8_MSG_BYTE_INIT:
        {
            // Get the I2C device path from u8x8 user_ptr
            const char* i2c_device = "/dev/i2c-1"; // Default
            
            // Open I2C device if not already open
            if (fd < 0) {
                fd = open(i2c_device, O_RDWR);
                if (fd < 0) {
                    perror("u8g2_hw_i2c: Failed to open I2C device");
                    return 0;
                }
                printf("u8g2_hw_i2c: Opened I2C device %s\n", i2c_device);
            }
            
            return 1;
        }
        
        case U8X8_MSG_BYTE_SET_DC:
            // Not used for I2C
            return 1;
            
        case U8X8_MSG_BYTE_START_TRANSFER:
        {
            device_address = u8x8_GetI2CAddress(u8x8) >> 1; // u8g2 uses shifted address
            
            // Set I2C slave address
            if (ioctl(fd, I2C_SLAVE, device_address) < 0) {
                perror("u8g2_hw_i2c: Failed to set I2C slave address");
                return 0;
            }
            
            return 1;
        }
        
        case U8X8_MSG_BYTE_SEND:
        {
            // Send data to I2C device
            uint8_t *data = (uint8_t *)arg_ptr;
            if (write(fd, data, arg_int) != arg_int) {
                perror("u8g2_hw_i2c: Failed to write to I2C device");
                return 0;
            }
            
            return 1;
        }
        
        case U8X8_MSG_BYTE_END_TRANSFER:
            // Nothing to do for I2C
            return 1;
            
        default:
            return 0;
    }
}
