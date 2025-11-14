#!/bin/bash
# Build nng library
# Called by download-dependencies.sh
# Expects: DEPS_DIR environment variable to be set

set -e

# Version
NNG_VERSION="1.11"

# Check environment
if [ -z "$DEPS_DIR" ]; then
    echo "ERROR: DEPS_DIR environment variable not set"
    echo "This script should be called from download-dependencies.sh"
    exit 1
fi

# Create work directory
WORK_DIR="/tmp/pluq_build_nng_$$"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

echo "Build directory: $WORK_DIR"
echo "Install directory: $DEPS_DIR"
echo ""

# =============================================================================
# Download nng
# =============================================================================

echo "Downloading nng v$NNG_VERSION..."
if command -v wget > /dev/null; then
    wget -q --show-progress -O nng.tar.gz "https://github.com/nanomsg/nng/archive/refs/tags/v${NNG_VERSION}.tar.gz" 2>&1 || echo "wget failed"
elif command -v curl > /dev/null; then
    curl -L --progress-bar -o nng.tar.gz "https://github.com/nanomsg/nng/archive/refs/tags/v${NNG_VERSION}.tar.gz" || echo "curl failed"
else
    echo "ERROR: Neither wget nor curl available"
    exit 1
fi

if [ ! -f nng.tar.gz ]; then
    echo "ERROR: nng download failed"
    exit 1
fi

tar xzf nng.tar.gz
NNG_SOURCE="$WORK_DIR/nng-${NNG_VERSION}"

# =============================================================================
# Build nng
# =============================================================================

echo "  Building nng..."
NNG_BUILD_DIR="$WORK_DIR/build-nng"
mkdir -p "$NNG_BUILD_DIR"
cd "$NNG_BUILD_DIR"

cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$DEPS_DIR" \
    -DBUILD_SHARED_LIBS=OFF \
    -DNNG_TESTS=OFF \
    -DNNG_TOOLS=OFF \
    -DNNG_ENABLE_TLS=OFF \
    "$NNG_SOURCE" > cmake.log 2>&1

if [ $? -ne 0 ]; then
    echo "ERROR: nng CMake configure failed. Last 20 lines:"
    tail -20 cmake.log
    exit 1
fi

cmake --build . --config Release -j$(nproc) > build.log 2>&1

if [ $? -ne 0 ]; then
    echo "ERROR: nng build failed. Last 20 lines:"
    tail -20 build.log
    exit 1
fi

cmake --install . > install.log 2>&1

if [ $? -ne 0 ]; then
    echo "ERROR: nng install failed. Last 20 lines:"
    tail -20 install.log
    exit 1
fi

echo "  ✓ nng built and installed"
echo ""

# =============================================================================
# Cleanup
# =============================================================================

cd /
rm -rf "$WORK_DIR"

echo "nng build complete!"
