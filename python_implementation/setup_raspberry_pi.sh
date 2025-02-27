#!/bin/bash
# Setup script for CarRadio SH1106 Display Adapter (Python Version) on Raspberry Pi

# Exit on error
set -e

echo "Setting up CarRadio SH1106 Display Adapter (Python Version) on Raspberry Pi..."

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
apt-get install -y i2c-tools python3-dev python3-pip libfreetype6-dev libjpeg-dev build-essential

# Install Python dependencies
echo "Installing Python dependencies..."
pip3 install luma.oled pillow

# Install the adapter package
echo "Installing the adapter package..."
pip3 install -e .

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

echo "After reboot, you can run the example with: python3 example.py"
echo "To check I2C devices: sudo i2cdetect -y 1"
