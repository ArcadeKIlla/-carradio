#!/bin/bash

# Test script for U8G2 integration
echo "Testing U8G2 integration..."

# Build the test program
echo "Building test_u8g2_vfd..."
mkdir -p build_test
cd build_test
cmake .. -DBUILD_TEST_U8G2_VFD=ON
make test_u8g2_vfd

# Run the test program
echo "Running test_u8g2_vfd..."
sudo ./test_u8g2_vfd

# Test with the main application
echo "Building main application with U8G2 display..."
cd ..
mkdir -p build
cd build
cmake ..
make -j4

echo "U8G2 integration test complete!"
echo "To run the main application with U8G2 display: sudo ./carradio"
