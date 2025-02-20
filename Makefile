CXX = g++
CXXFLAGS = -Wall -I/usr/include -std=c++11
LDFLAGS = -lwiringPi -li2c

# Source files
SOURCES = src/DisplayMgr.cpp \
         src/I2C.cpp \
         src/RotaryEncoder.cpp \
         src/SSD1306.cpp \
         src/SSD1306_LCD.cpp \
         src/DisplayTest.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Executable name
TARGET = carradio

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJECTS) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
