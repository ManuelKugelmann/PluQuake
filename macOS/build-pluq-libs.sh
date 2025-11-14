#!/bin/bash
# Build nng and flatcc libraries for macOS
# This follows the pattern used for Windows dependencies in this repo

set -e

# Versions
NNG_VERSION="1.11"
FLATCC_VERSION="0.6.1"

# Directories
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORK_DIR="${TMPDIR}pluq_build_$$"
OUTPUT_DIR="$SCRIPT_DIR/pluq"

# Detect architecture
ARCH=$(uname -m)
if [ "$ARCH" = "x86_64" ]; then
    ARCH_NAME="x86_64"
elif [ "$ARCH" = "arm64" ]; then
    ARCH_NAME="arm64"
else
    ARCH_NAME="$ARCH"
fi

echo "=========================================="
echo "  Building PluQ Dependencies for macOS"
echo "=========================================="
echo ""
echo "Architecture: $ARCH_NAME"
echo "nng version: $NNG_VERSION"
echo "flatcc version: $FLATCC_VERSION"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Check build tools
if ! command -v cmake > /dev/null; then
    echo "ERROR: cmake not found"
    echo "Install with: brew install cmake"
    exit 1
fi

if ! command -v make > /dev/null; then
    echo "ERROR: make not found"
    echo "Install Xcode Command Line Tools: xcode-select --install"
    exit 1
fi

echo "Build tools: OK (cmake, make, clang)"
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
if command -v curl > /dev/null; then
    curl -L --progress-bar -o nng.tar.gz "https://github.com/nanomsg/nng/archive/refs/tags/v${NNG_VERSION}.tar.gz"
else
    echo "ERROR: curl not available"
    exit 1
fi
tar xzf nng.tar.gz
NNG_SOURCE="$WORK_DIR/nng-${NNG_VERSION}"

# flatcc
echo "  Downloading flatcc v$FLATCC_VERSION..."
curl -L --progress-bar -o flatcc.tar.gz "https://github.com/dvidelabs/flatcc/archive/refs/tags/v${FLATCC_VERSION}.tar.gz"
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
    echo "Building $NAME ($ARCH_NAME)"
    echo "=========================================="

    local BUILD_DIR="$WORK_DIR/build-$NAME"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    echo "CMake configure..."
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$BUILD_DIR/install" \
        -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
        "${CMAKE_ARGS[@]}" \
        "$SOURCE_DIR" > cmake.log 2>&1

    if [ $? -ne 0 ]; then
        echo "CMake configure failed. Last 20 lines of log:"
        tail -20 cmake.log
        exit 1
    fi

    echo "CMake build..."
    cmake --build . --config Release -j$(sysctl -n hw.ncpu) > build.log 2>&1

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
echo "Building nng ($ARCH_NAME)"
echo "=========================================="
NNG_BUILD_DIR="$WORK_DIR/build-nng"
mkdir -p "$NNG_BUILD_DIR"
cd "$NNG_BUILD_DIR"

echo "CMake configure..."
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$NNG_BUILD_DIR/install" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
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
cmake --build . --config Release -j$(sysctl -n hw.ncpu) > build.log 2>&1

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
echo "Building flatcc ($ARCH_NAME)"
echo "=========================================="
FLATCC_BUILD_DIR="$WORK_DIR/build-flatcc"
mkdir -p "$FLATCC_BUILD_DIR"
cd "$FLATCC_BUILD_DIR"

echo "CMake configure..."
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$FLATCC_BUILD_DIR/install" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
    -DFLATCC_INSTALL=ON \
    -DFLATCC_RTONLY=OFF \
    "$FLATCC_SOURCE" > cmake.log 2>&1

if [ $? -ne 0 ]; then
    echo "CMake configure failed. Last 20 lines of log:"
    tail -20 cmake.log
    exit 1
fi

echo "CMake build..."
cmake --build . --config Release -j$(sysctl -n hw.ncpu) > build.log 2>&1

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

# Binaries (flatcc compiler)
echo "Copying flatcc compiler..."
mkdir -p "$OUTPUT_DIR/bin"
if [ -f "$FLATCC_BUILD_DIR/install/bin/flatcc" ]; then
    cp "$FLATCC_BUILD_DIR/install/bin/flatcc" "$OUTPUT_DIR/bin/"
    chmod +x "$OUTPUT_DIR/bin/flatcc"
fi

# Copy CMake files if they exist
if [ -d "$NNG_BUILD_DIR/install/lib/cmake" ]; then
    cp -r "$NNG_BUILD_DIR/install/lib/cmake" "$OUTPUT_DIR/lib/"
fi

# Get library sizes
NNG_SIZE=$(ls -lh "$OUTPUT_DIR/lib/libnng.a" | awk '{print $5}')
FLATCC_SIZE=$(ls -lh "$OUTPUT_DIR/lib/libflatccrt.a" | awk '{print $5}')

# Get compiler version
CLANG_VERSION=$(clang --version | head -1)

# Create README
cat > "$OUTPUT_DIR/README.txt" << EOF
# PluQ Precompiled Libraries (macOS $ARCH_NAME)

This directory contains precompiled libraries for PluQ IPC on macOS.

## Versions
- nng: $NNG_VERSION
- flatcc: $FLATCC_VERSION

## Build Information
**Build Date**: $(date +%Y-%m-%d)
**Platform**: macOS $ARCH_NAME
**Compiler**: $CLANG_VERSION
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
CFLAGS = -I../macOS/pluq/include
LDFLAGS = -L../macOS/pluq/lib -lnng -lflatccrt -lpthread
\`\`\`

### In Xcode
Add to "Header Search Paths":
  \$(PROJECT_DIR)/../macOS/pluq/include

Add to "Library Search Paths":
  \$(PROJECT_DIR)/../macOS/pluq/lib

Link with:
  libnng.a, libflatccrt.a, pthread

## Rebuild

To rebuild these libraries, run:
\`\`\`bash
cd macOS/
./build-pluq-libs.sh
\`\`\`

## Sources
- nng: https://github.com/nanomsg/nng
- flatcc: https://github.com/dvidelabs/flatcc

## Apple Silicon Notes

If building on Apple Silicon (M1/M2/M3), the libraries are built for arm64.
If building on Intel Mac, the libraries are built for x86_64.

For universal binaries, build separately on both architectures and use lipo:
\`\`\`bash
lipo -create libnng-arm64.a libnng-x86_64.a -output libnng.a
\`\`\`
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
echo "Libraries built for: $ARCH_NAME"
echo "  libnng.a ($NNG_SIZE)"
echo "  libflatccrt.a ($FLATCC_SIZE)"
echo ""
echo "Next steps:"
echo "1. Update Makefiles or Xcode projects to use macOS/pluq/lib and macOS/pluq/include"
echo "2. Commit the macOS/pluq directory to the repository"
echo ""
