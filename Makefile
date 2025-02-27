# Makefile for CarRadio SH1106 Display Adapter
# Optimized for Raspberry Pi

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -I. -I/usr/include -I/usr/local/include

# For Raspberry Pi, we need to link against the appropriate libraries
LDFLAGS = -L/usr/lib -L/usr/local/lib -lm -lpthread

# If you have installed the u8g2 library as a system library
# LDFLAGS += -lu8g2

# Source files
SOURCES = VFD_SH1106_Adapter.cpp example_raspberry_pi.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# If you're using the u8g2 source directly in your project
# Add all the necessary u8g2 source files
U8G2_DIR = ./u8g2_src
U8G2_SOURCES = $(wildcard $(U8G2_DIR)/*.c)
U8G2_OBJECTS = $(U8G2_SOURCES:.c=.o)

# Target executable
TARGET = example_raspberry_pi

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS) $(U8G2_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile C++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C source files (for u8g2)
%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJECTS) $(U8G2_OBJECTS) $(TARGET)

# Install (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Run the example
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean install run
