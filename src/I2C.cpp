#include "I2C.hpp"
#include <stdexcept>
#include <string.h>
#include <errno.h>

I2C::I2C() : _fd(-1), _devAddr(0), _isSetup(false) {
}

I2C::~I2C() {
    stop();
}

bool I2C::begin(uint8_t devAddr) {
    int error;
    return begin(devAddr, error);
}

bool I2C::begin(uint8_t devAddr, int& error) {
    return begin(devAddr, "/dev/i2c-1", error);  // Default I2C bus on Raspberry Pi
}

bool I2C::begin(uint8_t devAddr, const char* path, int& error) {
    _devAddr = devAddr;
    
    // Open the I2C bus
    _fd = open(path, O_RDWR);
    if (_fd < 0) {
        error = errno;
        return false;
    }

    // Set the I2C slave address
    if (ioctl(_fd, I2C_SLAVE, _devAddr) < 0) {
        error = errno;
        close(_fd);
        _fd = -1;
        return false;
    }

    _isSetup = true;
    return true;
}

void I2C::stop() {
    if (_fd >= 0) {
        close(_fd);
        _fd = -1;
    }
    _isSetup = false;
}

bool I2C::isAvailable() {
    return _isSetup && (_fd >= 0);
}

bool I2C::writeByte(uint8_t byte) {
    if (!_isSetup) return false;

    return write(_fd, &byte, 1) == 1;
}

bool I2C::writeByte(uint8_t regAddr, uint8_t byte) {
    if (!_isSetup) return false;

    uint8_t buffer[2] = { regAddr, byte };
    return write(_fd, buffer, 2) == 2;
}

bool I2C::writeWord(uint8_t regAddr, uint16_t word, bool swap) {
    if (!_isSetup) return false;

    uint8_t buffer[3];
    buffer[0] = regAddr;
    if (swap) {
        buffer[1] = word >> 8;
        buffer[2] = word & 0xFF;
    }
    else {
        buffer[1] = word & 0xFF;
        buffer[2] = word >> 8;
    }

    return write(_fd, buffer, 3) == 3;
}

bool I2C::readByte(uint8_t& byte) {
    if (!_isSetup) return false;

    return read(_fd, &byte, 1) == 1;
}

bool I2C::readByte(uint8_t regAddr, uint8_t& byte) {
    if (!_isSetup) return false;

    if (write(_fd, &regAddr, 1) != 1) return false;
    return read(_fd, &byte, 1) == 1;
}

bool I2C::readWord(uint8_t regAddr, uint16_t& word, bool swap) {
    if (!_isSetup) return false;

    if (write(_fd, &regAddr, 1) != 1) return false;

    uint8_t buffer[2];
    if (read(_fd, buffer, 2) != 2) return false;

    if (swap)
        word = (buffer[0] << 8) | buffer[1];
    else
        word = (buffer[1] << 8) | buffer[0];

    return true;
}

bool I2C::readWord(uint8_t regAddr, int16_t& word, bool swap) {
    uint16_t uword;
    bool success = readWord(regAddr, uword, swap);
    if (success)
        word = static_cast<int16_t>(uword);
    return success;
}

bool I2C::readBlock(uint8_t regAddr, uint8_t size, i2c_block_t& block) {
    if (!_isSetup || size > 32) return false;

    if (write(_fd, &regAddr, 1) != 1) return false;
    return read(_fd, block, size) == size;
}

bool I2C::writeBlock(uint8_t regAddr, uint8_t size, i2c_block_t block) {
    if (!_isSetup || size > 32) return false;

    uint8_t buffer[33];
    buffer[0] = regAddr;
    memcpy(buffer + 1, block, size);

    return write(_fd, buffer, size + 1) == size + 1;
}

bool I2C::readByte(uint8_t regAddr, unsigned char* byte) {
    return readByte(regAddr, reinterpret_cast<uint8_t&>(*byte));
}

bool I2C::readBlock(uint8_t regAddr, uint8_t size, unsigned char* block) {
    return readBlock(regAddr, size, reinterpret_cast<i2c_block_t&>(*block));
}
