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

# Find ALSA
pkg_check_modules(ALSA REQUIRED alsa)

# Find RTL-SDR
pkg_check_modules(RTLSDR REQUIRED librtlsdr)

# Find additional required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Add minmea source files
set(MINMEA_SOURCES
    lib/minmea/minmea.c
)

# Add DSP filter source files
set(DSP_SOURCES
    lib/dsp/FineTuner.cpp
    lib/dsp/LowPassFilterFirIQ.cpp
    lib/dsp/DownsampleFilter.cpp
    lib/dsp/HighPassFilterIir.cpp
    lib/dsp/LowPassFilterRC.cpp
)

set(SOURCES
    ${MINMEA_SOURCES}
    ${DSP_SOURCES}
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
    src/VhfDecode.cpp
    src/FmDecode.cpp
    src/CANBusMgr.cpp
    src/DTCcodes.cpp
    src/Wranger2010.cpp
    src/GMLAN.cpp
    src/OBD2.cpp
    src/DuppaEncoder.cpp
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
    ${ALSA_INCLUDE_DIRS}
    ${RTLSDR_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lib/minmea  # For minmea headers
    ${SQLITE3_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/lib/dsp  # For DSP filter headers
)

# Add gpiod library
find_library(GPIOD_LIBRARY gpiod)
if(NOT GPIOD_LIBRARY)
    message(FATAL_ERROR "gpiod library not found")
endif()

target_link_libraries(carradio
    PRIVATE
    ${PIGPIO_LIBRARIES}
    gpiod
    ${ALSA_LIBRARIES}
    ${RTLSDR_LIBRARIES}
    i2c
    ${SQLITE3_LIBRARIES}
    m  # Math library for DSP
)
