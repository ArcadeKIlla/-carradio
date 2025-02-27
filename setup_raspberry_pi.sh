#!/bin/bash
# Setup script for CarRadio SH1106 Display Adapter on Raspberry Pi

# Exit on error
set -e

echo "Setting up CarRadio SH1106 Display Adapter on Raspberry Pi..."

# Check if running as root
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (use sudo)"
  exit 1
fi

# Enable I2C if not already enabled
echo "Enabling I2C interface..."
if ! grep -q "^dtparam=i2c_arm=on" /boot/config.txt; then
  echo "dtparam=i2c_arm=on" >> /boot/config.txt
  echo "I2C enabled in /boot/config.txt. A reboot will be needed."
  REBOOT_NEEDED=1
else
  echo "I2C already enabled in /boot/config.txt"
fi

# Install required packages
echo "Installing required packages..."
apt-get update
apt-get install -y i2c-tools libi2c-dev build-essential git

# Clone u8g2 library if not already present
if [ ! -d "u8g2" ]; then
  echo "Cloning u8g2 library..."
  git clone https://github.com/olikraus/u8g2.git
else
  echo "u8g2 library already exists. Updating..."
  cd u8g2
  git pull
  cd ..
fi

# Build the example
echo "Building example..."
make clean
make

echo "Setup complete!"

if [ "$REBOOT_NEEDED" = "1" ]; then
  echo "A reboot is required for I2C changes to take effect."
  read -p "Would you like to reboot now? (y/n) " -n 1 -r
  echo
  if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Rebooting..."
    reboot
  else
    echo "Please reboot manually when convenient."
  fi
fi

echo "After reboot, you can run the example with: ./example_raspberry_pi"
echo "To check I2C devices: sudo i2cdetect -y 1"
