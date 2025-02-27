# CarRadio SH1106 Display Adapter

This project provides an adapter to replace the VFD display in the original CarRadio project with an SH1106 OLED display. The adapter maintains the same interface as the original VFD class while translating commands to the SH1106 OLED display using the u8g2 library.

## Raspberry Pi Setup

This adapter is designed to work on a Raspberry Pi. Follow these steps to set up your Raspberry Pi for use with the SH1106 OLED display:

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

# Install I2C tools and development libraries
sudo apt-get install -y i2c-tools libi2c-dev

# Install build tools if not already installed
sudo apt-get install -y build-essential cmake git
```

### 3. Install the U8g2 Library

```bash
# Clone the u8g2 library
git clone https://github.com/olikraus/u8g2.git

# Navigate to the u8g2 directory
cd u8g2/

# Copy the library to your project or install system-wide
# Option 1: Copy to your project (recommended)
cp -r csrc /path/to/your/project/u8g2_src

# Option 2: Install system-wide (advanced)
# Follow the instructions in the u8g2 repository
```

### 4. Verify I2C Connection

```bash
# Scan for I2C devices to verify your display is connected
sudo i2cdetect -y 1
```

You should see your SH1106 display listed (typically at address 0x3C or 0x3D).

### 5. Wiring the SH1106 OLED Display to Raspberry Pi

Connect your SH1106 OLED display to the Raspberry Pi using these connections:

| SH1106 Pin | Raspberry Pi Pin |
|------------|------------------|
| VCC        | 3.3V (Pin 1)     |
| GND        | Ground (Pin 6)   |
| SCL        | GPIO 3 (SCL, Pin 5) |
| SDA        | GPIO 2 (SDA, Pin 3) |

## Using the Adapter

The adapter is designed to be a drop-in replacement for the original VFD class. Simply replace instances of the VFD class with VFD_SH1106_Adapter in your code:

```cpp
// Original code
VFD* display = new VFD();
display->begin("/dev/ttyS0", B19200);

// New code with adapter
VFD_SH1106_Adapter* display = new VFD_SH1106_Adapter();
display->begin("/dev/i2c-1", 0x3C); // Use the I2C bus and address
```

## Troubleshooting

### Display Not Working

1. Check I2C address with `sudo i2cdetect -y 1`
2. Verify wiring connections
3. Ensure I2C is enabled in raspi-config
4. Check power supply - the SH1106 requires stable power

### Display Shows Garbled Text

1. Verify the I2C clock speed - try a lower speed if needed
2. Check for interference on the I2C lines
3. Keep I2C cables short and away from noise sources

## License

This project is licensed under the same terms as the original CarRadio project.

## Credits

- Original CarRadio project: https://github.com/vinthewrench/carradio
- U8g2 library: https://github.com/olikraus/u8g2
