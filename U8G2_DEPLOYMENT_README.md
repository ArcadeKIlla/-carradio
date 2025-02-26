# U8G2 Library Deployment Guide

This guide provides instructions for deploying the U8G2 graphics library on your Raspberry Pi for the CarRadio project.

## Prerequisites

- Raspberry Pi with Raspbian/Raspberry Pi OS
- Git installed
- Internet connection
- CarRadio project cloned from GitHub

## Deployment Steps

### 1. Clone the CarRadio Repository (if not already done)

```bash
git clone https://github.com/ArcadeKIlla/-carradio.git
cd carradio
```

### 2. Make the Deployment Scripts Executable

```bash
chmod +x deploy_u8g2_library.sh
chmod +x update_cmake_u8g2_path.sh
```

### 3. Deploy the U8G2 Library

Run the deployment script to clone and install the U8G2 library:

```bash
./deploy_u8g2_library.sh
```

This script will:
- Clone the U8G2 repository
- Copy the necessary source files to ~/u8g2/csrc
- Clean up temporary files

### 4. Update the CMakeLists.txt File

Run the update script to modify the U8G2 library path in CMakeLists.txt:

```bash
./update_cmake_u8g2_path.sh
```

This script will:
- Create a backup of the original CMakeLists.txt
- Update the U8G2_DIR path to point to ~/u8g2/csrc

### 5. Build the CarRadio Project

```bash
mkdir -p build
cd build
cmake ..
make -j4
```

### 6. Run the CarRadio Application

```bash
sudo ./carradio
```

## Troubleshooting

If you encounter any issues during the build process:

1. **CMake Error: Cannot find source file**
   - Ensure the U8G2 library is properly installed at ~/u8g2/csrc
   - Check that the CMakeLists.txt file has been updated with the correct path

2. **Compilation Errors**
   - Make sure all dependencies are installed
   - Check the CMake output for specific error messages

3. **Linker Errors**
   - Verify that all required libraries are installed on your system

## Additional Resources

- [U8G2 GitHub Repository](https://github.com/olikraus/u8g2)
- [U8G2 Documentation](https://github.com/olikraus/u8g2/wiki)
- [CarRadio Project Documentation](https://github.com/ArcadeKIlla/-carradio)
