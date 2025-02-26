cmake_minimum_required(VERSION 3.0)

if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt." )
endif()

project(carradio)

SET (CMAKE_C_COMPILER             "clang")
SET (CMAKE_CXX_COMPILER            "clang++")
set(CMAKE_CXX_FLAGS "-Wall -std=c++17 -O2 -ffast-math -ftree-vectorize ${EXTRA_FLAGS}")

find_package(Threads)
find_package(PkgConfig)
find_package(ALSA REQUIRED)

# Find RTL-SDR library (for linking).
pkg_check_modules(PKG_RTLSDR librtlsdr)
find_path(RTLSDR_INCLUDE_DIR rtl-sdr.h
          HINT ${PKG_RTLSDR_INCLUDE_DIRS})
find_library(RTLSDR_LIBRARY librtlsdr.a
             HINT ${PKG_RTLSDR_LIBRARY_DIRS})

# Find libusb
pkg_check_modules(PKG_LIBUSB libusb-1.0)
find_path(LIBUSB_INCLUDE_DIR libusb.h
          HINT ${PKG_LIBUSB_INCLUDE_DIRS}
          PATH_SUFFIXES libusb-1.0)
find_library(LIBUSB_LIBRARY usb-1.0
             HINT ${PKG_LIBUSB_LIBRARY_DIRS})

if(RTLSDR_INCLUDE_DIR AND RTLSDR_LIBRARY)
    message(STATUS "Found librtlsdr: ${RTLSDR_INCLUDE_DIR}, ${RTLSDR_LIBRARY}")
else()
    message(WARNING "Can not find Osmocom RTL-SDR library")
    message("Try again with environment variable PKG_CONFIG_PATH")
    message("or with -DRTLSDR_INCLUDE_DIR=/path/rtlsdr/include")
    message("        -DRTLSDR_LIBRARY=/path/rtlsdr/lib/librtlsdr.a")
endif()

set(RTLSDR_INCLUDE_DIRS ${RTLSDR_INCLUDE_DIR} ${LIBUSB_INCLUDE_DIR})
set(RTLSDR_LIBRARIES    ${RTLSDR_LIBRARY} ${LIBUSB_LIBRARY})

# Set language for minmea source files
set_source_files_properties(src/minmea.c PROPERTIES LANGUAGE C)

# Add u8g2 library
set(U8G2_DIR C:/Users/Nate/u8g2/csrc)
set(U8G2_SOURCES
    ${U8G2_DIR}/u8g2_bitmap.c
    ${U8G2_DIR}/u8g2_box.c
    ${U8G2_DIR}/u8g2_buffer.c
    ${U8G2_DIR}/u8g2_circle.c
    ${U8G2_DIR}/u8g2_cleardisplay.c
    ${U8G2_DIR}/u8g2_d_memory.c
    ${U8G2_DIR}/u8g2_d_setup.c
    ${U8G2_DIR}/u8g2_font.c
    ${U8G2_DIR}/u8g2_fonts.c
    ${U8G2_DIR}/u8g2_hvline.c
    ${U8G2_DIR}/u8g2_input_value.c
    ${U8G2_DIR}/u8g2_intersection.c
    ${U8G2_DIR}/u8g2_kerning.c
    ${U8G2_DIR}/u8g2_line.c
    ${U8G2_DIR}/u8g2_ll_hvline.c
    ${U8G2_DIR}/u8g2_message.c
    ${U8G2_DIR}/u8g2_polygon.c
    ${U8G2_DIR}/u8g2_selection_list.c
    ${U8G2_DIR}/u8g2_setup.c
    ${U8G2_DIR}/u8x8_8x8.c
    ${U8G2_DIR}/u8x8_byte.c
    ${U8G2_DIR}/u8x8_cad.c
    ${U8G2_DIR}/u8x8_capture.c
    ${U8G2_DIR}/u8x8_d_ssd1306_128x64_noname.c
    ${U8G2_DIR}/u8x8_debounce.c
    ${U8G2_DIR}/u8x8_display.c
    ${U8G2_DIR}/u8x8_fonts.c
    ${U8G2_DIR}/u8x8_gpio.c
    ${U8G2_DIR}/u8x8_input_value.c
    ${U8G2_DIR}/u8x8_message.c
    ${U8G2_DIR}/u8x8_selection_list.c
    ${U8G2_DIR}/u8x8_setup.c
    ${U8G2_DIR}/u8x8_string.c
    ${U8G2_DIR}/u8x8_u16toa.c
    ${U8G2_DIR}/u8x8_u8toa.c
)

add_executable(carradio
    src/main.cpp
    src/PiCarMgr.cpp
    src/DisplayMgr.cpp
    src/SSD1306_LCD.cpp
    src/SSD1306.cpp
    src/SH1106.cpp
    src/I2C.cpp
    src/DuppaEncoder.cpp
    src/DuppaKnob.cpp
    src/DuppaLEDRing.cpp
    src/Filter.cpp
    src/GenericEncoder.cpp
    src/PiCarMgr.cpp
    src/RadioMgr.cpp
    src/GPSmgr.cpp
    src/PiCarCAN.cpp
    src/VFD.cpp
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
    src/minmea.c
    src/u8g2_hw_i2c.cpp
    ${U8G2_SOURCES}
)

set_target_properties(carradio PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
)

set_target_properties(carradio PROPERTIES
    OUTPUT_NAME "carradio"
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(
    ${RTLSDR_INCLUDE_DIRS}
    ${U8G2_DIR}
    ${ALSA_INCLUDE_DIRS}
    ${EXTRA_INCLUDES}
)

link_directories(
    ${RTLSDR_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS}
    ${EXTRA_INCLUDES}
)

target_link_libraries(carradio
    PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    ${RTLSDR_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${EXTRA_LIBS}
    Threads::Threads
    gpiod
    sqlite3
    rt
)

target_include_directories(carradio
    PRIVATE
    src
)

set(CMAKE_BINARY_DIR "bin")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Only copy assets if the directory exists
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/Assets)
    add_custom_target(copy_assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Assets ${CMAKE_BINARY_DIR}
    )
    add_dependencies(carradio copy_assets)
endif()
