SET (CMAKE_C_COMPILER             "clang")
SET (CMAKE_CXX_COMPILER            "clang++")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 -O2 -ffast-math -ftree-vectorize ${EXTRA_FLAGS}")

cmake_minimum_required(VERSION 3.0)
project(carradio)

# Find required packages
find_package(PkgConfig REQUIRED)

# Try pkg-config first, but fall back to manual paths if not found
pkg_check_modules(PIGPIO pigpio)
if(NOT PIGPIO_FOUND)
    message(STATUS "pigpio not found via pkg-config, using default paths")
    set(PIGPIO_LIBRARIES "pigpio")
    set(PIGPIO_INCLUDE_DIRS "/usr/local/include")
endif()

set(SOURCES
    src/main.cpp
    src/DisplayMgr.cpp
    src/I2C.cpp
    src/RotaryEncoder.cpp
    src/SSD1306.cpp
    src/SSD1306_LCD.cpp
    src/PiCarMgr.cpp
    src/RadioMgr.cpp
    src/GPSmgr.cpp
    src/PiCarCAN.cpp
    src/VFD.cpp
    src/DuppaKnob.cpp
    src/DuppaLEDRing.cpp
    src/ErrorMgr.cpp
    src/FrameDB.cpp
    src/AudioOutput.cpp
    src/AudioLineInput.cpp
    src/AirplayInput.cpp
    src/W1Mgr.cpp
    src/ArgononeFan.cpp
    src/CPUInfo.cpp
    src/PiCarDB.cpp
    src/DTCManager.cpp
    src/TimeStamp.cpp
    src/RtlSdr.cpp
    src/dbuf.cpp
)

add_executable(carradio ${SOURCES})

set_target_properties(carradio PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
)

target_include_directories(carradio
    PRIVATE
    src
    ${PIGPIO_INCLUDE_DIRS}
)

find_library(GPIOD_LIBRARY gpiod)
if(NOT GPIOD_LIBRARY)
    message(FATAL_ERROR "gpiod library not found")
endif()

target_link_libraries(carradio
    PRIVATE
    ${PIGPIO_LIBRARIES}
    ${GPIOD_LIBRARY}
    i2c
)
