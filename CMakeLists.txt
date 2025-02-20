SET (CMAKE_C_COMPILER             "clang")
SET (CMAKE_CXX_COMPILER            "clang++")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 -O2 -ffast-math -ftree-vectorize ${EXTRA_FLAGS}")

cmake_minimum_required(VERSION 3.0)
project(carradio)

set(SOURCES
    src/DisplayMgr.cpp
    src/I2C.cpp
    src/RotaryEncoder.cpp
    src/SSD1306.cpp
    src/SSD1306_LCD.cpp
    src/DisplayTest.cpp
)

add_executable(carradio ${SOURCES})

set_target_properties(carradio PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
)

target_link_libraries(carradio
    PRIVATE
    wiringPi
    i2c
)

target_include_directories(carradio
    PRIVATE
    src
)
