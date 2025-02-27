#!/usr/bin/env python3
"""
VFD Bridge - Connects CarRadio app to SH1106 OLED display
This script acts as a bridge between the CarRadio C++ application and the Python SH1106 OLED adapter.
It listens for commands on a socket and translates them to calls to the VFD_SH1106_Adapter class.
"""

import socket
import sys
import threading
import time
import json
import logging
from vfd_sh1106_adapter import VFD_SH1106_Adapter

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("/tmp/vfd_bridge.log"),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger("VFD_Bridge")

# Socket configuration
SOCKET_PATH = "/tmp/carradio_vfd.sock"

class VFDBridge:
    """Bridge between CarRadio app and SH1106 OLED display"""
    
    def __init__(self):
        """Initialize the VFD bridge"""
        self.display = VFD_SH1106_Adapter()
        self.display.begin("/dev/i2c-1")  # Specify the I2C device path
        self.running = False
        self.socket = None
        self.clients = []
        
        # Display startup message
        self.display.write("VFD Bridge")
        self.display.set_cursor(0, 1)  # Set cursor to row 1
        self.display.write("Starting...")
        time.sleep(1)
        self.display.clear_screen()
        
    def start(self):
        """Start the VFD bridge server"""
        self.running = True
        
        # Create Unix domain socket
        try:
            # Remove socket if it already exists
            import os
            if os.path.exists(SOCKET_PATH):
                os.unlink(SOCKET_PATH)
                
            self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.socket.bind(SOCKET_PATH)
            self.socket.listen(5)
            
            # Set socket permissions so CarRadio app can connect
            os.chmod(SOCKET_PATH, 0o777)
            
            logger.info(f"VFD Bridge started, listening on {SOCKET_PATH}")
            self.display.write("VFD Bridge Ready")
            
            # Accept connections in a loop
            while self.running:
                client, _ = self.socket.accept()
                self.clients.append(client)
                client_thread = threading.Thread(target=self.handle_client, args=(client,))
                client_thread.daemon = True
                client_thread.start()
                
        except Exception as e:
            logger.error(f"Error starting VFD Bridge: {str(e)}")
            self.display.write("Error:")
            self.display.set_cursor(0, 1)  # Set cursor to row 1
            self.display.write(str(e)[:20])  # Show first 20 chars of error
            
        finally:
            self.stop()
            
    def stop(self):
        """Stop the VFD bridge server"""
        self.running = False
        
        # Close all client connections
        for client in self.clients:
            try:
                client.close()
            except:
                pass
        self.clients = []
        
        # Close server socket
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            
        logger.info("VFD Bridge stopped")
        
    def handle_client(self, client):
        """Handle communication with a client"""
        try:
            buffer = b""
            while self.running:
                data = client.recv(1024)
                if not data:
                    break
                    
                buffer += data
                
                # Process complete commands (newline-terminated)
                while b'\n' in buffer:
                    command, buffer = buffer.split(b'\n', 1)
                    self.process_command(command.decode('utf-8', errors='ignore').strip())
                    
        except Exception as e:
            logger.error(f"Error handling client: {str(e)}")
            
        finally:
            try:
                client.close()
            except:
                pass
            if client in self.clients:
                self.clients.remove(client)
                
    def process_command(self, command):
        """Process a command from the client"""
        logger.info(f"Received command: {command}")
        
        try:
            # Parse command as JSON
            cmd_data = json.loads(command)
            cmd_type = cmd_data.get('cmd')
            
            if cmd_type == 'write':
                # Write text to display
                text = cmd_data.get('text', '')
                line = cmd_data.get('line', 0)
                self.display.set_cursor(0, line)  # Set cursor to specified line
                self.display.write(text)
                
            elif cmd_type == 'clear':
                # Clear display
                self.display.clear_screen()
                
            elif cmd_type == 'printLines':
                # Print multiple lines
                lines = cmd_data.get('lines', [])
                self.display.clear_screen()
                for i, line in enumerate(lines):
                    self.display.set_cursor(0, i)
                    self.display.write(line)
                
            elif cmd_type == 'drawScrollBar':
                # Draw scroll bar
                position = cmd_data.get('position', 0)
                size = cmd_data.get('size', 1)
                self.display.draw_scroll_bar(0, size, position)
                
            elif cmd_type == 'setFont':
                # Set font size
                size = cmd_data.get('size', 1)
                font_map = {
                    0: self.display.FONT_MINI,
                    1: self.display.FONT_5x7,
                    2: self.display.FONT_10x14
                }
                font = font_map.get(size, self.display.FONT_MINI)
                self.display.set_font(font)
                
            else:
                logger.warning(f"Unknown command type: {cmd_type}")
                
        except json.JSONDecodeError:
            # Handle plain text commands (for backward compatibility)
            if command.startswith("write:"):
                parts = command.split(":", 1)
                if len(parts) > 1:
                    self.display.write(parts[1])
            elif command == "clear":
                self.display.clear_screen()
            else:
                logger.warning(f"Invalid command format: {command}")
                
        except Exception as e:
            logger.error(f"Error processing command: {str(e)}")
            
def main():
    """Main function"""
    bridge = VFDBridge()
    
    # Handle Ctrl+C gracefully
    try:
        bridge.start()
    except KeyboardInterrupt:
        logger.info("Keyboard interrupt received, stopping...")
    finally:
        bridge.stop()
        
if __name__ == "__main__":
    main()
