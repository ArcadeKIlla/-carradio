# CarRadio SH1106 Display Adapter (Python Version)

This project provides a Python-based adapter to replace the VFD display in the original CarRadio project with an SH1106 OLED display. The adapter uses the `luma.oled` library, which is specifically designed for driving OLED displays on the Raspberry Pi.

## Features

- Full compatibility with the original VFD interface
- Support for different font sizes
- Scrollbar functionality
- Multi-line text display
- Row-based display with columns

## Prerequisites

- Raspberry Pi (any model with I2C)
- Python 3.6 or newer
- SH1106 OLED display (I2C interface)

## Installation

### 1. Enable I2C on Raspberry Pi

```bash
# Open the Raspberry Pi configuration tool
sudo raspi-config

# Navigate to: Interface Options > I2C > Yes to enable I2C
# Reboot when prompted or run:
sudo reboot
```

### 2. Install Required Packages

```bash
# Update package lists
sudo apt-get update

# Install I2C tools and Python development libraries
sudo apt-get install -y i2c-tools python3-dev python3-pip libfreetype6-dev libjpeg-dev build-essential

# Install the luma.oled library and other dependencies
pip3 install luma.oled pillow
```

### 3. Clone the Repository

```bash
# Clone the repository
git clone https://github.com/yourusername/carradio_sh1106_python.git
cd carradio_sh1106_python
```

### 4. Run the Example

```bash
# Run the example script
python3 example.py
```

## Wiring the SH1106 OLED Display to Raspberry Pi

Connect your SH1106 OLED display to the Raspberry Pi using these connections:

| SH1106 Pin | Raspberry Pi Pin |
|------------|------------------|
| VCC        | 3.3V (Pin 1)     |
| GND        | Ground (Pin 6)   |
| SCL        | GPIO 3 (SCL, Pin 5) |
| SDA        | GPIO 2 (SDA, Pin 3) |

## Usage

```python
from vfd_sh1106_adapter import VFD_SH1106_Adapter

# Create the adapter
display = VFD_SH1106_Adapter()

# Initialize with I2C on Raspberry Pi
# 0x3C is the default I2C address for most SH1106 displays
display.begin("/dev/i2c-1", 0x3C)

# Set maximum brightness
display.set_brightness(7)

# Display a simple message
display.set_cursor(0, 10)
display.set_font(VFD_SH1106_Adapter.FONT_5x7)
display.write("CarRadio")

# Clear the screen
display.clear_screen()

# Display multiple lines
lines = ["Line 1", "Line 2", "Line 3", "Line 4"]
display.print_lines(10, 10, lines, 0, 4, VFD_SH1106_Adapter.FONT_MINI)

# Clean up
display.set_power_on(False)
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.
