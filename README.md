# CarRadio SH1106 Display Adapter

This project provides an adapter to replace the VFD display in the original CarRadio project with an SH1106 OLED display. The adapter maintains the same interface as the original VFD class while translating commands to the SH1106 OLED display using the u8g2 library.

## Raspberry Pi Setup

This adapter is designed to work on a Raspberry Pi. Follow these steps to set up your Raspberry Pi for use with the SH1106 OLED display:

### Quick Setup (Recommended)

```bash
# Clone the repository if you haven't already
git clone https://github.com/ArcadeKIlla/-carradio.git carradio_sh1106
cd carradio_sh1106

# Checkout the sh1106-adapter branch
git checkout sh1106-adapter

# Run the setup script (requires sudo)
chmod +x setup_raspberry_pi.sh
sudo ./setup_raspberry_pi.sh
```

### Manual Setup

If you prefer to set up manually, follow these steps:

#### 1. Enable I2C on Raspberry Pi

```bash
# Open the Raspberry Pi configuration tool
sudo raspi-config

# Navigate to: Interface Options > I2C > Yes to enable I2C
# Reboot when prompted or run:
sudo reboot
```

#### 2. Install Required Packages

```bash
# Update package lists
sudo apt-get update

# Install I2C tools and development libraries
sudo apt-get install -y i2c-tools libi2c-dev

# Install build tools if not already installed
sudo apt-get install -y build-essential git
```

#### 3. Install the U8g2 Library

```bash
# Clone the u8g2 library into your project directory
git clone https://github.com/olikraus/u8g2.git
```

#### 4. Build the Example

```bash
# Build the example
make clean
make

# Run the example
./example_raspberry_pi
```

#### 5. Verify I2C Connection

```bash
# Scan for I2C devices to verify your display is connected
sudo i2cdetect -y 1
```

You should see your SH1106 display listed (typically at address 0x3C or 0x3D).

### Wiring the SH1106 OLED Display to Raspberry Pi

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

### Build Issues

1. Make sure you've cloned the u8g2 library into the project directory
2. Check that the u8g2 directory structure matches what's expected in the Makefile
3. If you get include errors, verify the paths in the Makefile and header files

### Display Shows Garbled Text

1. Verify the I2C clock speed - try a lower speed if needed
2. Check for interference on the I2C lines
3. Keep I2C cables short and away from noise sources

## License

This project is licensed under the same terms as the original CarRadio project.

## Credits

- Original CarRadio project: https://github.com/vinthewrench/carradio
- U8g2 library: https://github.com/olikraus/u8g2
