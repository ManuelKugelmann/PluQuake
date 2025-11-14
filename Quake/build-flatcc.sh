#!/bin/bash
# Build flatcc library
# Called by download-dependencies.sh
# Expects: DEPS_DIR environment variable to be set

set -e

# Version
FLATCC_VERSION="0.6.1"

# Check environment
if [ -z "$DEPS_DIR" ]; then
    echo "ERROR: DEPS_DIR environment variable not set"
    echo "This script should be called from download-dependencies.sh"
    exit 1
fi

# Create work directory
WORK_DIR="/tmp/pluq_build_flatcc_$$"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

echo "Build directory: $WORK_DIR"
echo "Install directory: $DEPS_DIR"
echo ""

# =============================================================================
# Download flatcc
# =============================================================================

echo "Downloading flatcc v$FLATCC_VERSION..."
if command -v wget > /dev/null; then
    wget -q --show-progress -O flatcc.tar.gz "https://github.com/dvidelabs/flatcc/archive/refs/tags/v${FLATCC_VERSION}.tar.gz" 2>&1 || echo "wget failed"
elif command -v curl > /dev/null; then
    curl -L --progress-bar -o flatcc.tar.gz "https://github.com/dvidelabs/flatcc/archive/refs/tags/v${FLATCC_VERSION}.tar.gz" || echo "curl failed"
else
    echo "ERROR: Neither wget nor curl available"
    exit 1
fi

if [ ! -f flatcc.tar.gz ]; then
    echo "ERROR: flatcc download failed"
    exit 1
fi

tar xzf flatcc.tar.gz
FLATCC_SOURCE="$WORK_DIR/flatcc-${FLATCC_VERSION}"

# =============================================================================
# Build flatcc
# =============================================================================

echo "  Building flatcc..."
FLATCC_BUILD_DIR="$WORK_DIR/build-flatcc"
mkdir -p "$FLATCC_BUILD_DIR"
cd "$FLATCC_BUILD_DIR"

cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$DEPS_DIR" \
    -DFLATCC_INSTALL=ON \
    -DFLATCC_RTONLY=ON \
    "$FLATCC_SOURCE" > cmake.log 2>&1

if [ $? -ne 0 ]; then
    echo "ERROR: flatcc CMake configure failed. Last 20 lines:"
    tail -20 cmake.log
    exit 1
fi

cmake --build . --config Release -j$(nproc) > build.log 2>&1

if [ $? -ne 0 ]; then
    echo "ERROR: flatcc build failed. Last 20 lines:"
    tail -20 build.log
    exit 1
fi

cmake --install . > install.log 2>&1

if [ $? -ne 0 ]; then
    echo "ERROR: flatcc install failed. Last 20 lines:"
    tail -20 install.log
    exit 1
fi

echo "  ✓ flatcc built and installed"
echo ""

# =============================================================================
# Cleanup
# =============================================================================

cd /
rm -rf "$WORK_DIR"

echo "flatcc build complete!"
