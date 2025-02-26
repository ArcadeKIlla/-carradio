cmake_minimum_required(VERSION 3.10)
project(test_u8g2_vfd)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/u8g2/csrc
)

# Find all U8G2 source files
file(GLOB U8G2_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/u8g2/csrc/*.c
)

# Add the executable
add_executable(test_u8g2_vfd
    src/test_u8g2_vfd.cpp
    src/I2C.cpp
    src/VFD.cpp
    src/u8g2_hw_i2c.cpp
    ${U8G2_SOURCES}
)

# Link libraries
target_link_libraries(test_u8g2_vfd
    pthread
)
