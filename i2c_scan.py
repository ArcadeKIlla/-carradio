#!/usr/bin/env python3
"""
This script scans the I2C bus for connected devices
"""

import smbus
import time
import sys

def scan_i2c_bus(bus_number=1):
    """Scan I2C bus for devices"""
    print(f"Scanning I2C bus {bus_number}...")
    
    try:
        bus = smbus.SMBus(bus_number)
        
        found_devices = []
        
        for device in range(3, 128):
            try:
                bus.read_byte(device)
                print(f"Found device at address: 0x{device:02X}")
                found_devices.append(device)
            except Exception as e:
                # No device at this address
                pass
            
            time.sleep(0.001)
        
        if not found_devices:
            print("No I2C devices found")
        else:
            print(f"Found {len(found_devices)} I2C devices")
            
        return found_devices
    except Exception as e:
        print(f"Error accessing I2C bus {bus_number}: {e}")
        return []

if __name__ == "__main__":
    # Default to bus 1, but allow command-line override
    bus_number = 1
    if len(sys.argv) > 1:
        try:
            bus_number = int(sys.argv[1])
        except ValueError:
            print(f"Invalid bus number: {sys.argv[1]}")
            sys.exit(1)
    
    scan_i2c_bus(bus_number)
