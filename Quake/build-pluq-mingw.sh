#!/bin/bash
# Build nng and flatcc for MinGW (cross-compilation from Linux)
# This creates MinGW-compatible libraries for Windows builds

set -e

ARCH="${1:-x86}"  # x86 or x64
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build-pluq-mingw-$ARCH"
INSTALL_DIR="$SCRIPT_DIR/../Windows/pluq-mingw"

# Set MinGW compiler based on architecture
if [ "$ARCH" = "x86" ]; then
    MINGW_PREFIX="i686-w64-mingw32"
    TOOLCHAIN_FILE="$BUILD_DIR/toolchain-mingw32.cmake"
else
    MINGW_PREFIX="x86_64-w64-mingw32"
    TOOLCHAIN_FILE="$BUILD_DIR/toolchain-mingw64.cmake"
fi

echo "=========================================="
echo "  Building PluQ libraries for MinGW"
echo "  Architecture: $ARCH"
echo "  Compiler: $MINGW_PREFIX"
echo "=========================================="

# Check if MinGW is installed
if ! command -v ${MINGW_PREFIX}-gcc &> /dev/null; then
    echo "ERROR: MinGW compiler not found: ${MINGW_PREFIX}-gcc"
    echo "Install with: sudo apt-get install mingw-w64"
    exit 1
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Create CMake toolchain file for MinGW cross-compilation
cat > "$TOOLCHAIN_FILE" << EOF
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER ${MINGW_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${MINGW_PREFIX}-windres)
set(CMAKE_FIND_ROOT_PATH /usr/${MINGW_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

echo ""
echo "Step 1/2: Building nng..."
echo "=========================================="

# Clone nng if not already present
if [ ! -d "nng" ]; then
    git clone --depth 1 -b v2.0.0-alpha.6 https://github.com/nanomsg/nng.git
fi

# Build nng
mkdir -p nng/build-mingw
cd nng/build-mingw
cmake \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DNNG_TESTS=OFF \
    -DNNG_TOOLS=OFF \
    -DNNG_ENABLE_TLS=OFF \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    ..
make -j$(nproc)
make install
cd ../..

echo ""
echo "Step 2/2: Building flatcc..."
echo "=========================================="

# Clone flatcc if not already present
if [ ! -d "flatcc" ]; then
    git clone --depth 1 https://github.com/dvidelabs/flatcc.git
fi

# Build flatcc runtime only
mkdir -p flatcc/build-mingw
cd flatcc/build-mingw
cmake \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE=Release \
    -DFLATCC_INSTALL=ON \
    -DFLATCC_RTONLY=ON \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    ..
make -j$(nproc)
make install
cd ../..

echo ""
echo "=========================================="
echo "  Build Complete"
echo "=========================================="
echo "Libraries installed to: $INSTALL_DIR"
echo ""
echo "Files created:"
ls -lh "$INSTALL_DIR/lib/"
echo ""
echo "To use these libraries, update Makefile.w32/w64 to use:"
echo "  CFLAGS += -I../Windows/pluq-mingw/include"
echo "  LDFLAGS += -L../Windows/pluq-mingw/lib"
