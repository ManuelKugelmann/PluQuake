#!/bin/bash
# Build nng and flatcc libraries for Linux
# This follows the pattern used for Windows dependencies in this repo

set -e

# Versions
NNG_VERSION="1.11"
FLATCC_VERSION="0.6.1"

# Directories
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORK_DIR="${TMPDIR:-/tmp}/pluq_build_$$"
OUTPUT_DIR="$SCRIPT_DIR/pluq"

echo "=========================================="
echo "  Building PluQ Dependencies for Linux"
echo "=========================================="
echo ""
echo "nng version: $NNG_VERSION"
echo "flatcc version: $FLATCC_VERSION"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Check build tools
if ! command -v cmake > /dev/null; then
    echo "ERROR: cmake not found"
    echo "Install with: sudo apt-get install cmake build-essential"
    exit 1
fi

if ! command -v make > /dev/null; then
    echo "ERROR: make not found"
    echo "Install with: sudo apt-get install build-essential"
    exit 1
fi

echo "Build tools: OK (cmake, make, gcc)"
echo ""

# Create directories
mkdir -p "$WORK_DIR"
mkdir -p "$OUTPUT_DIR/include"
mkdir -p "$OUTPUT_DIR/lib"

# Download sources
echo "Downloading sources..."
cd "$WORK_DIR"

# nng
echo "  Downloading nng v$NNG_VERSION..."
if command -v wget > /dev/null; then
    wget -q --show-progress -O nng.tar.gz "https://github.com/nanomsg/nng/archive/refs/tags/v${NNG_VERSION}.tar.gz"
elif command -v curl > /dev/null; then
    curl -L --progress-bar -o nng.tar.gz "https://github.com/nanomsg/nng/archive/refs/tags/v${NNG_VERSION}.tar.gz"
else
    echo "ERROR: Neither wget nor curl available"
    exit 1
fi
tar xzf nng.tar.gz
NNG_SOURCE="$WORK_DIR/nng-${NNG_VERSION}"

# flatcc
echo "  Downloading flatcc v$FLATCC_VERSION..."
if command -v wget > /dev/null; then
    wget -q --show-progress -O flatcc.tar.gz "https://github.com/dvidelabs/flatcc/archive/refs/tags/v${FLATCC_VERSION}.tar.gz"
elif command -v curl > /dev/null; then
    curl -L --progress-bar -o flatcc.tar.gz "https://github.com/dvidelabs/flatcc/archive/refs/tags/v${FLATCC_VERSION}.tar.gz"
else
    echo "ERROR: Neither wget nor curl available"
    exit 1
fi
tar xzf flatcc.tar.gz
FLATCC_SOURCE="$WORK_DIR/flatcc-${FLATCC_VERSION}"

echo ""

# Build function
build_library() {
    local NAME=$1
    local SOURCE_DIR=$2
    shift 2
    local CMAKE_ARGS=("$@")

    echo "=========================================="
    echo "Building $NAME"
    echo "=========================================="

    local BUILD_DIR="$WORK_DIR/build-$NAME"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    echo "CMake configure..."
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$BUILD_DIR/install" \
        "${CMAKE_ARGS[@]}" \
        "$SOURCE_DIR" > cmake.log 2>&1

    if [ $? -ne 0 ]; then
        echo "CMake configure failed. Last 20 lines of log:"
        tail -20 cmake.log
        exit 1
    fi

    echo "CMake build..."
    cmake --build . --config Release -j$(nproc) > build.log 2>&1

    if [ $? -ne 0 ]; then
        echo "CMake build failed. Last 20 lines of log:"
        tail -20 build.log
        exit 1
    fi

    echo "CMake install..."
    cmake --install . > install.log 2>&1

    if [ $? -ne 0 ]; then
        echo "CMake install failed. Last 20 lines of log:"
        tail -20 install.log
        exit 1
    fi

    echo "✓ $NAME built successfully"
    echo ""
}

# Build nng
echo "=========================================="
echo "Building nng"
echo "=========================================="
NNG_BUILD_DIR="$WORK_DIR/build-nng"
mkdir -p "$NNG_BUILD_DIR"
cd "$NNG_BUILD_DIR"

echo "CMake configure..."
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$NNG_BUILD_DIR/install" \
    -DBUILD_SHARED_LIBS=OFF \
    -DNNG_TESTS=OFF \
    -DNNG_TOOLS=OFF \
    -DNNG_ENABLE_TLS=OFF \
    "$NNG_SOURCE" > cmake.log 2>&1

if [ $? -ne 0 ]; then
    echo "CMake configure failed. Last 20 lines of log:"
    tail -20 cmake.log
    exit 1
fi

echo "CMake build..."
cmake --build . --config Release -j$(nproc) > build.log 2>&1

if [ $? -ne 0 ]; then
    echo "CMake build failed. Last 20 lines of log:"
    tail -20 build.log
    exit 1
fi

echo "CMake install..."
cmake --install . > install.log 2>&1

if [ $? -ne 0 ]; then
    echo "CMake install failed. Last 20 lines of log:"
    tail -20 install.log
    exit 1
fi

echo "✓ nng built successfully"
echo ""

# Build flatcc
echo "=========================================="
echo "Building flatcc"
echo "=========================================="
FLATCC_BUILD_DIR="$WORK_DIR/build-flatcc"
mkdir -p "$FLATCC_BUILD_DIR"
cd "$FLATCC_BUILD_DIR"

echo "CMake configure..."
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$FLATCC_BUILD_DIR/install" \
    -DFLATCC_INSTALL=ON \
    -DFLATCC_RTONLY=ON \
    "$FLATCC_SOURCE" > cmake.log 2>&1

if [ $? -ne 0 ]; then
    echo "CMake configure failed. Last 20 lines of log:"
    tail -20 cmake.log
    exit 1
fi

echo "CMake build..."
cmake --build . --config Release -j$(nproc) > build.log 2>&1

if [ $? -ne 0 ]; then
    echo "CMake build failed. Last 20 lines of log:"
    tail -20 build.log
    exit 1
fi

echo "CMake install..."
cmake --install . > install.log 2>&1

if [ $? -ne 0 ]; then
    echo "CMake install failed. Last 20 lines of log:"
    tail -20 install.log
    exit 1
fi

echo "✓ flatcc built successfully"
echo ""


# Copy files to output directory
echo "=========================================="
echo "Copying files to $OUTPUT_DIR"
echo "=========================================="

# Headers
echo "Copying headers..."
cp -r "$NNG_BUILD_DIR/install/include/nng" "$OUTPUT_DIR/include/"
cp -r "$FLATCC_BUILD_DIR/install/include/flatcc" "$OUTPUT_DIR/include/"

# Libraries
echo "Copying libraries..."
cp "$NNG_BUILD_DIR/install/lib/libnng.a" "$OUTPUT_DIR/lib/"
cp "$FLATCC_BUILD_DIR/install/lib/libflatccrt.a" "$OUTPUT_DIR/lib/"

# Copy CMake files if they exist
if [ -d "$NNG_BUILD_DIR/install/lib/cmake" ]; then
    cp -r "$NNG_BUILD_DIR/install/lib/cmake" "$OUTPUT_DIR/lib/"
fi

# Get library sizes
NNG_SIZE=$(ls -lh "$OUTPUT_DIR/lib/libnng.a" | awk '{print $5}')
FLATCC_SIZE=$(ls -lh "$OUTPUT_DIR/lib/libflatccrt.a" | awk '{print $5}')

# Create README
cat > "$OUTPUT_DIR/README.txt" << EOF
# PluQ Precompiled Libraries (Linux x86_64)

This directory contains precompiled libraries for PluQ IPC on Linux.

## Versions
- nng: $NNG_VERSION
- flatcc: $FLATCC_VERSION

## Build Information
**Build Date**: $(date +%Y-%m-%d)
**Platform**: Linux x86_64
**Compiler**: gcc $(gcc -dumpversion)
**Build Type**: Release (static libraries)

## Contents

### lib/
- libnng.a ($NNG_SIZE) - nng v$NNG_VERSION (nanomsg-next-generation) static library
- libflatccrt.a ($FLATCC_SIZE) - flatcc v$FLATCC_VERSION (FlatBuffers for C) runtime library
- cmake/ - CMake package configuration files

### include/
- nng/ - nng headers (v$NNG_VERSION)
- flatcc/ - flatcc headers (v$FLATCC_VERSION)

## Features

### nng v$NNG_VERSION
- Protocol support: REQ/REP, PUB/SUB, PUSH/PULL
- Transport: TCP/IP (localhost)
- Thread-safe
- Zero-copy message passing

### flatcc v$FLATCC_VERSION
- FlatBuffers serialization for C
- Runtime-only build (no compiler tools)
- Zero-copy deserialization
- Schema-based type safety

## Usage

### In Makefiles
\`\`\`makefile
CFLAGS = -I../Linux/pluq/include
LDFLAGS = -L../Linux/pluq/lib -lnng -lflatccrt -lpthread
\`\`\`

### Test Programs
All test programs in pluq-deployment/ use these prebuilt libraries.

## Rebuild

To rebuild these libraries, run:
\`\`\`bash
cd Linux/
./build-pluq-libs.sh
\`\`\`

## Sources
- nng: https://github.com/nanomsg/nng
- flatcc: https://github.com/dvidelabs/flatcc

## Verified

These libraries have been tested and verified with:
- ✅ Input channel (PUSH/PULL) - 100% success rate
- ✅ Gameplay channel (PUB/SUB) - 98% reception rate
- ✅ FlatBuffers serialization/deserialization
- ✅ Real-time performance at 60 FPS

See IPC_CHANNEL_TEST_REPORT.md for full test results.
EOF

# Cleanup
echo "Cleaning up build directory..."
cd /
rm -rf "$WORK_DIR"

echo ""
echo "=========================================="
echo "  Build Complete!"
echo "=========================================="
echo ""
echo "Output directory: $OUTPUT_DIR"
echo ""
echo "Libraries built:"
echo "  libnng.a ($NNG_SIZE)"
echo "  libflatccrt.a ($FLATCC_SIZE)"
echo ""
echo "Next steps:"
echo "1. Update Makefiles to use Linux/pluq/lib and Linux/pluq/include"
echo "2. Commit the Linux/pluq directory to the repository"
echo ""
