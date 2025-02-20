#!/bin/bash

# Create lib directory if it doesn't exist
mkdir -p lib/minmea

# Download minmea files
wget -O lib/minmea/minmea.h https://raw.githubusercontent.com/kosma/minmea/master/minmea.h
wget -O lib/minmea/minmea.c https://raw.githubusercontent.com/kosma/minmea/master/minmea.c

echo "Minmea files downloaded successfully!"
