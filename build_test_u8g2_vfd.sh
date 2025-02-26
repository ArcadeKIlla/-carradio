#!/bin/bash

# Create build directory if it doesn't exist
mkdir -p build_test

# Build the test program
cd build_test
cmake -DCMAKE_BUILD_TYPE=Debug .. -f ../test_u8g2_vfd.cmake
make

# Run the test program
echo "Build complete. Run the test with: sudo ./test_u8g2_vfd"
