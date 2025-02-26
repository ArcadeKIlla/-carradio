# Manual Fix Instructions for U8G2 Library Deployment

If the automated scripts don't work, follow these manual steps to fix the U8G2 library deployment issues.

## Step 1: Edit CMakeLists.txt

Open the CMakeLists.txt file in your favorite text editor:

```bash
nano CMakeLists.txt
```

Find this line (around line 47):

```cmake
set(U8G2_DIR C:/Users/Nate/u8g2/csrc)
```

Replace it with:

```cmake
set(U8G2_DIR /home/nate/u8g2/csrc)
```

Save and exit the editor (in nano: Ctrl+O, Enter, Ctrl+X).

## Step 2: Install the U8G2 Library

Run these commands to install the U8G2 library:

```bash
# Create the directory
mkdir -p /home/nate/u8g2

# Clone the repository
git clone https://github.com/olikraus/u8g2.git /tmp/u8g2

# Copy the source files
cp -r /tmp/u8g2/csrc /home/nate/u8g2/

# Clean up
rm -rf /tmp/u8g2
```

## Step 3: Build the Project

```bash
# Create the build directory
mkdir -p build

# Navigate to the build directory
cd build

# Run CMake
cmake ..

# Build the project
make -j4
```

## Step 4: Run the Application

```bash
sudo ./carradio
```

## Troubleshooting

If you still encounter issues:

1. Verify the U8G2 library is installed correctly:
   ```bash
   ls -la /home/nate/u8g2/csrc
   ```

2. Check that the path in CMakeLists.txt is correct:
   ```bash
   grep "U8G2_DIR" CMakeLists.txt
   ```

3. Make sure all dependencies are installed:
   ```bash
   sudo apt-get update
   sudo apt-get install build-essential cmake git
   ```

4. Check the CMake error messages for specific issues.
