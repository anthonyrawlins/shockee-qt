#!/bin/bash

# Ubuntu 24.04 LTS build script for Shockee
# This script builds a .deb package for apt installation

set -e

echo "Building Shockee for Ubuntu 24.04 LTS..."

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Run this script from the project root."
    exit 1
fi

# Install build dependencies
echo "Installing build dependencies..."
sudo apt update
sudo apt install -y debhelper-compat cmake qt6-base-dev qt6-serialport-dev libqt6printsupport6-dev pkg-config build-essential

# Clean previous builds
echo "Cleaning previous builds..."
rm -rf build debian/shockee

# Create build directory and build the application
echo "Building application..."
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

# Build the Debian package
echo "Building Debian package..."
dpkg-buildpackage -us -uc -b

echo "Build completed successfully!"
echo "Package created: ../shockee_1.0.0-1_amd64.deb"
echo ""
echo "To install the package:"
echo "  sudo dpkg -i ../shockee_1.0.0-1_amd64.deb"
echo "  sudo apt-get install -f  # Fix any dependency issues"
echo ""
echo "To test the package:"
echo "  shockee"
echo ""
echo "Note: Install shockee-simulator package first for testing without hardware"