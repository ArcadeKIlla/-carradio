# CarRadio VFD Bridge for SH1106 OLED Display

This package provides a bridge between the CarRadio C++ application and the SH1106 OLED display. It allows the CarRadio app to send display commands to the OLED display via a Unix domain socket.

## Components

1. **vfd_bridge.py**: Python script that acts as a bridge between the CarRadio app and the SH1106 OLED display
2. **VFD_Socket_Adapter.hpp**: C++ header file that provides an interface for the CarRadio app to communicate with the bridge
3. **carradio-vfd-bridge.service**: Systemd service file for running the bridge on startup
4. **test_vfd_bridge.py**: Test script for verifying that the bridge works correctly

## Installation

### 1. Set up the Python environment

```bash
# Navigate to your CarRadio project directory
cd ~/carradio_fresh

# Create a virtual environment if not already created
python3 -m venv venv

# Activate the virtual environment
source venv/bin/activate

# Install required packages
pip install luma.oled Pillow
```

### 2. Copy the bridge files

```bash
# Copy the bridge files to the Python implementation directory
cp vfd_bridge.py ~/carradio_fresh/python_implementation/
cp test_vfd_bridge.py ~/carradio_fresh/python_implementation/
```

### 3. Set up the systemd service

```bash
# Copy the service file to the systemd directory
sudo cp carradio-vfd-bridge.service /etc/systemd/system/

# Reload systemd
sudo systemctl daemon-reload

# Enable the service to start on boot
sudo systemctl enable carradio-vfd-bridge.service

# Start the service
sudo systemctl start carradio-vfd-bridge.service
```

### 4. Integrate with the CarRadio app

1. Copy the `VFD_Socket_Adapter.hpp` file to your CarRadio source directory
2. Modify your CarRadio app to use the `VFD_Socket_Adapter` class instead of the original VFD class

Example:

```cpp
// Include the adapter header
#include "VFD_Socket_Adapter.hpp"

// Create an instance of the adapter
VFD_Socket_Adapter vfd;

// Initialize the adapter
if (!vfd.begin()) {
    std::cerr << "Failed to initialize VFD adapter" << std::endl;
    return 1;
}

// Use the adapter to display text
vfd.clear();
vfd.write("Hello, CarRadio!");
```

## Testing

You can test the bridge using the provided test script:

```bash
# Activate the virtual environment
source ~/carradio_fresh/venv/bin/activate

# Run the test script
python ~/carradio_fresh/python_implementation/test_vfd_bridge.py
```

## Troubleshooting

If you encounter issues with the bridge, check the log file:

```bash
# View the bridge log
cat /tmp/vfd_bridge.log

# View the systemd service log
sudo journalctl -u carradio-vfd-bridge.service
```

## License

This software is provided under the MIT License. See the LICENSE file for details.
