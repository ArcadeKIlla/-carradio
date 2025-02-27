#pragma once

#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <json/json.h> // You may need to install libjsoncpp-dev

/**
 * VFD_Socket_Adapter - Adapter for communicating with the Python VFD bridge
 * This class provides an interface compatible with the original VFD class
 * but sends commands to the Python bridge over a Unix domain socket.
 */
class VFD_Socket_Adapter {
public:
    VFD_Socket_Adapter() : socket_fd_(-1) {}
    
    ~VFD_Socket_Adapter() {
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
    }
    
    bool begin() {
        // Connect to the Unix domain socket
        socket_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (socket_fd_ < 0) {
            std::cerr << "Error creating socket" << std::endl;
            return false;
        }
        
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, "/tmp/carradio_vfd.sock", sizeof(addr.sun_path) - 1);
        
        if (connect(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "Error connecting to socket" << std::endl;
            close(socket_fd_);
            socket_fd_ = -1;
            return false;
        }
        
        return true;
    }
    
    void clear() {
        Json::Value cmd;
        cmd["cmd"] = "clear";
        sendCommand(cmd);
    }
    
    void write(const std::string& text, int line = 0) {
        Json::Value cmd;
        cmd["cmd"] = "write";
        cmd["text"] = text;
        cmd["line"] = line;
        sendCommand(cmd);
    }
    
    void printLines(const std::vector<std::string>& lines) {
        Json::Value cmd;
        cmd["cmd"] = "printLines";
        
        Json::Value linesArray(Json::arrayValue);
        for (const auto& line : lines) {
            linesArray.append(line);
        }
        cmd["lines"] = linesArray;
        
        sendCommand(cmd);
    }
    
    void drawScrollBar(float position, float size) {
        Json::Value cmd;
        cmd["cmd"] = "drawScrollBar";
        cmd["position"] = position;
        cmd["size"] = size;
        sendCommand(cmd);
    }
    
    void setFont(int size) {
        Json::Value cmd;
        cmd["cmd"] = "setFont";
        cmd["size"] = size;
        sendCommand(cmd);
    }
    
private:
    int socket_fd_;
    
    void sendCommand(const Json::Value& cmd) {
        if (socket_fd_ < 0) {
            // Try to reconnect if socket is closed
            if (!begin()) {
                std::cerr << "Failed to reconnect to VFD bridge" << std::endl;
                return;
            }
        }
        
        Json::StreamWriterBuilder writer;
        std::string cmdStr = Json::writeString(writer, cmd) + "\n";
        
        if (write(socket_fd_, cmdStr.c_str(), cmdStr.length()) < 0) {
            std::cerr << "Error sending command to VFD bridge" << std::endl;
            close(socket_fd_);
            socket_fd_ = -1;
        }
    }
};
