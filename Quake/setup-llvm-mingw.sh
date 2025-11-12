#!/bin/bash
# Setup LLVM-MinGW for cross-compilation
# This script downloads and installs LLVM-MinGW if not already present

set -e

LLVM_MINGW_VERSION="20251104"
LLVM_MINGW_ARCH="x86_64"
INSTALL_DIR="/opt/llvm-mingw"
TOOLCHAIN_NAME="llvm-mingw-${LLVM_MINGW_VERSION}-ucrt-ubuntu-22.04-${LLVM_MINGW_ARCH}"
TOOLCHAIN_DIR="$INSTALL_DIR/$TOOLCHAIN_NAME"

echo "=========================================="
echo "  LLVM-MinGW Setup for Cross-Compilation"
echo "=========================================="
echo ""
echo "This script will install LLVM-MinGW $LLVM_MINGW_VERSION"
echo "Installation directory: $INSTALL_DIR"
echo ""

# Check if already installed
if [ -d "$TOOLCHAIN_DIR" ]; then
    echo "✓ LLVM-MinGW already installed at $TOOLCHAIN_DIR"
    echo ""
    echo "Verifying installation..."
    if [ -f "$TOOLCHAIN_DIR/bin/x86_64-w64-mingw32-gcc" ]; then
        echo "✓ x86_64-w64-mingw32-gcc found"
    fi
    if [ -f "$TOOLCHAIN_DIR/bin/i686-w64-mingw32-gcc" ]; then
        echo "✓ i686-w64-mingw32-gcc found"
    fi
    echo ""
    echo "LLVM-MinGW is ready for use."
    echo "PATH: $TOOLCHAIN_DIR/bin"
    exit 0
fi

# Create installation directory
echo "Creating installation directory..."
sudo mkdir -p "$INSTALL_DIR"

# Download LLVM-MinGW
echo "Downloading LLVM-MinGW $LLVM_MINGW_VERSION..."
DOWNLOAD_URL="https://github.com/mstorsjo/llvm-mingw/releases/download/$LLVM_MINGW_VERSION/llvm-mingw-$LLVM_MINGW_VERSION-ucrt-ubuntu-22.04-$LLVM_MINGW_ARCH.tar.xz"

wget -q --show-progress -O /tmp/llvm-mingw.tar.xz "$DOWNLOAD_URL"

if [ ! -f /tmp/llvm-mingw.tar.xz ]; then
    echo "ERROR: Failed to download LLVM-MinGW"
    exit 1
fi

# Extract
echo ""
echo "Extracting LLVM-MinGW..."
sudo tar -xf /tmp/llvm-mingw.tar.xz -C "$INSTALL_DIR/"
rm /tmp/llvm-mingw.tar.xz

# Verify installation
echo ""
echo "Verifying installation..."
if [ -d "$TOOLCHAIN_DIR" ]; then
    echo "✓ LLVM-MinGW extracted successfully"
else
    echo "ERROR: Installation directory not found"
    exit 1
fi

if [ -f "$TOOLCHAIN_DIR/bin/x86_64-w64-mingw32-gcc" ]; then
    echo "✓ x86_64-w64-mingw32-gcc installed"
else
    echo "ERROR: x86_64-w64-mingw32-gcc not found"
    exit 1
fi

if [ -f "$TOOLCHAIN_DIR/bin/i686-w64-mingw32-gcc" ]; then
    echo "✓ i686-w64-mingw32-gcc installed"
else
    echo "ERROR: i686-w64-mingw32-gcc not found"
    exit 1
fi

echo ""
echo "=========================================="
echo "  Installation Complete!"
echo "=========================================="
echo ""
echo "LLVM-MinGW $LLVM_MINGW_VERSION installed to:"
echo "  $TOOLCHAIN_DIR"
echo ""
echo "To use LLVM-MinGW for cross-compilation:"
echo "  ./build_cross_win64-sdl2.sh       # Build 64-bit Windows binary"
echo "  ./build_cross_win32-sdl2.sh       # Build 32-bit Windows binary"
echo ""
echo "The cross-compilation scripts are already configured to use this toolchain."
echo ""
