CXX = clang++
CXXFLAGS = -Wall -I/usr/include -std=c++17 -I.
LDFLAGS = -lwiringPi -li2c -lpthread -lrtlsdr -lasound -lm

# Source files
SOURCES = src/main.cpp \
         src/DisplayMgr.cpp \
         src/I2C.cpp \
         src/RotaryEncoder.cpp \
         src/SSD1306.cpp \
         src/SSD1306_LCD.cpp \
         src/ErrorMgr.cpp \
         src/PiCarMgr.cpp \
         src/DuppaLEDRing.cpp \
         src/DuppaKnob.cpp \
         src/VFD.cpp \
         src/RadioMgr.cpp \
         src/GPSmgr.cpp \
         src/AudioOutput.cpp \
         src/CANBusMgr.cpp \
         src/PiCarDB.cpp \
         src/TimeStamp.cpp \
         src/Filter.cpp \
         src/FmDecode.cpp \
         src/RtlSdr.cpp \
         src/VhfDecode.cpp \
         src/W1Mgr.cpp \
         src/ArgononeFan.cpp \
         src/TempSensor.cpp \
         src/dbuf.cpp \
         src/tranmerc.cpp \
         src/minmea.c

# Object files
OBJECTS = $(SOURCES:.cpp=.o) $(SOURCES:.c=.o)

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

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJECTS) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
