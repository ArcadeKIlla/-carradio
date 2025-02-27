#!/usr/bin/env python3
"""
Test script for VFD Bridge
This script tests the VFD Bridge by sending commands to it via a Unix domain socket.
"""

import socket
import sys
import time
import json

# Socket configuration
SOCKET_PATH = "/tmp/carradio_vfd.sock"

def send_command(cmd_data):
    """Send a command to the VFD Bridge"""
    try:
        # Convert command to JSON string
        cmd_str = json.dumps(cmd_data) + "\n"
        
        # Create socket
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(SOCKET_PATH)
        
        # Send command
        sock.sendall(cmd_str.encode('utf-8'))
        
        # Close socket
        sock.close()
        
        return True
    except Exception as e:
        print(f"Error sending command: {str(e)}")
        return False

def main():
    """Main function"""
    print("Testing VFD Bridge...")
    
    # Test clear command
    print("Clearing display...")
    send_command({"cmd": "clear"})
    time.sleep(1)
    
    # Test write command
    print("Writing text...")
    send_command({"cmd": "write", "text": "VFD Bridge Test", "line": 0})
    send_command({"cmd": "write", "text": "Line 2", "line": 1})
    time.sleep(2)
    
    # Test printLines command
    print("Printing multiple lines...")
    send_command({"cmd": "printLines", "lines": ["Line 1", "Line 2", "Line 3", "Line 4"]})
    time.sleep(2)
    
    # Test drawScrollBar command
    print("Drawing scroll bar...")
    send_command({"cmd": "drawScrollBar", "position": 0.25, "size": 0.5})
    time.sleep(2)
    
    # Test setFont command
    print("Setting font size...")
    send_command({"cmd": "setFont", "size": 2})
    send_command({"cmd": "clear"})
    send_command({"cmd": "write", "text": "Big Font", "line": 0})
    time.sleep(2)
    
    # Reset to normal font
    send_command({"cmd": "setFont", "size": 1})
    send_command({"cmd": "clear"})
    
    # Final message
    send_command({"cmd": "write", "text": "Test Complete", "line": 0})
    send_command({"cmd": "write", "text": "Ready for CarRadio", "line": 1})
    
    print("Test complete!")

if __name__ == "__main__":
    main()
