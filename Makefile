# Makefile for CarRadio SH1106 Display Adapter
# Optimized for Raspberry Pi

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -I. -I/usr/include -I/usr/local/include

# Add U8g2 library path - update this path if needed
U8G2_PATH = ./u8g2
CXXFLAGS += -I$(U8G2_PATH)/cppsrc -I$(U8G2_PATH)/csrc

# For Raspberry Pi, we need to link against the appropriate libraries
LDFLAGS = -L/usr/lib -L/usr/local/lib -lm -lpthread

# Source files
SOURCES = VFD_SH1106_Adapter.cpp example_raspberry_pi.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# U8g2 C++ source files
U8G2_CPP_SOURCES = $(U8G2_PATH)/cppsrc/U8g2lib.cpp
U8G2_CPP_OBJECTS = $(U8G2_CPP_SOURCES:.cpp=.o)

# U8g2 C source files
U8G2_C_SOURCES = $(wildcard $(U8G2_PATH)/csrc/*.c)
U8G2_C_OBJECTS = $(U8G2_C_SOURCES:.c=.o)

# Target executable
TARGET = example_raspberry_pi

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS) $(U8G2_CPP_OBJECTS) $(U8G2_C_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile C++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C source files (for u8g2)
%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJECTS) $(U8G2_CPP_OBJECTS) $(U8G2_C_OBJECTS) $(TARGET)

# Install (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Run the example
run: $(TARGET)
	./$(TARGET)

# Setup target to clone u8g2 library
setup:
	git clone https://github.com/olikraus/u8g2.git

.PHONY: all clean install run setup
