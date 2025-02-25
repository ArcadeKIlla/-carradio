#!/bin/bash
# Build script for the Car Radio project

echo "=== Cleaning up any previous build ==="
rm -rf build
mkdir -p build
cd build || exit

echo "=== Running CMake ==="
cmake ..

echo "=== Building the project ==="
make -j4

echo "=== Build completed ==="
if [ -f bin/carradio ]; then
  echo "Success! The executable is located at: $(pwd)/bin/carradio"
else
  echo "Error: Build failed, executable not found."
  exit 1
fi
