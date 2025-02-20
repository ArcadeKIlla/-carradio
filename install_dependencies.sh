#!/bin/bash

# Update package lists
sudo apt-get update

# Install ALSA development libraries
sudo apt-get install -y libasound2-dev

# Install RTL-SDR libraries and tools
sudo apt-get install -y librtlsdr-dev rtl-sdr

# Install pigpio
sudo apt-get install -y pigpio python-pigpio python3-pigpio

# Install libgpiod
sudo apt-get install -y gpiod libgpiod-dev

# Install build essentials and CMake if not already installed
sudo apt-get install -y build-essential cmake

# Enable and start pigpio daemon
sudo systemctl enable pigpiod
sudo systemctl start pigpiod

echo "All dependencies installed successfully!"
