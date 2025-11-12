#!/bin/bash
# Build nng and flatcc for MinGW (cross-compilation from Linux)
# This creates MinGW-compatible libraries for Windows builds using LLVM-MinGW (UCRT)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="$SCRIPT_DIR/build-pluq-mingw"
OUTPUT_DIR="$SCRIPT_DIR/pluq"

# LLVM-MinGW version and download URL
LLVM_MINGW_VERSION="20251104"
LLVM_MINGW_ARCH="x86_64"  # We'll build both x86 and x64 libraries with this

echo "=========================================="
echo "  Building PluQ for MinGW with UCRT"
echo "=========================================="
echo ""
echo "This uses LLVM-MinGW which has UCRT support"
echo "required by nng 1.11 (and 2.x)"
echo ""

# Create work directory
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

# Download LLVM-MinGW if not present
TOOLCHAIN_DIR="$WORK_DIR/llvm-mingw-$LLVM_MINGW_VERSION-ucrt-ubuntu-$LLVM_MINGW_ARCH"
if [ ! -d "$TOOLCHAIN_DIR" ]; then
    echo "Downloading LLVM-MinGW toolchain..."
    LLVM_MINGW_URL="https://github.com/mstorsjo/llvm-mingw/releases/download/$LLVM_MINGW_VERSION/llvm-mingw-$LLVM_MINGW_VERSION-ucrt-ubuntu-22.04-$LLVM_MINGW_ARCH.tar.xz"
    wget -O llvm-mingw.tar.xz "$LLVM_MINGW_URL"
    tar -xf llvm-mingw.tar.xz
    mv llvm-mingw-*-ucrt-ubuntu-*-$LLVM_MINGW_ARCH "$TOOLCHAIN_DIR"
    rm llvm-mingw.tar.xz
fi

export PATH="$TOOLCHAIN_DIR/bin:$PATH"

# Verify toolchain
if ! command -v x86_64-w64-mingw32-gcc &> /dev/null; then
    echo "ERROR: LLVM-MinGW toolchain not found in PATH"
    exit 1
fi

echo "✓ LLVM-MinGW toolchain ready"
x86_64-w64-mingw32-gcc --version | head -1

# Function to build for an architecture
build_arch() {
    local ARCH=$1
    local TARGET_PREFIX=""

    if [ "$ARCH" = "x86" ]; then
        TARGET_PREFIX="i686-w64-mingw32"
    else
        TARGET_PREFIX="x86_64-w64-mingw32"
    fi

    echo ""
    echo "=========================================="
    echo "  Building for $ARCH"
    echo "=========================================="

    # Create CMake toolchain file
    local TOOLCHAIN_FILE="$WORK_DIR/toolchain-$ARCH.cmake"
    cat > "$TOOLCHAIN_FILE" << EOF
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER $TARGET_PREFIX-gcc)
set(CMAKE_CXX_COMPILER $TARGET_PREFIX-g++)
set(CMAKE_RC_COMPILER $TARGET_PREFIX-windres)
set(CMAKE_FIND_ROOT_PATH /usr/$TARGET_PREFIX)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

    # Build nng
    echo ""
    echo "Step 1/2: Building nng..."

    if [ ! -d "nng" ]; then
        git clone --depth 1 -b v1.11 https://github.com/nanomsg/nng.git
    fi

    mkdir -p "nng/build-$ARCH"
    cd "nng/build-$ARCH"

    cmake \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_FLAGS="-Wno-error" \
        -DBUILD_SHARED_LIBS=OFF \
        -DNNG_TESTS=OFF \
        -DNNG_TOOLS=OFF \
        -DNNG_ENABLE_TLS=OFF \
        -DCMAKE_INSTALL_PREFIX="$WORK_DIR/install-$ARCH" \
        ..

    make -j$(nproc)
    make install
    cd ../..

    # Build flatcc
    echo ""
    echo "Step 2/2: Building flatcc..."

    if [ ! -d "flatcc" ]; then
        git clone --depth 1 https://github.com/dvidelabs/flatcc.git
    fi

    mkdir -p "flatcc/build-$ARCH"
    cd "flatcc/build-$ARCH"

    cmake \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_FLAGS="-Wno-error=implicit-int-conversion-on-negation -Wno-error" \
        -DFLATCC_INSTALL=ON \
        -DFLATCC_RTONLY=ON \
        -DCMAKE_INSTALL_PREFIX="$WORK_DIR/install-$ARCH" \
        -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY="$WORK_DIR/flatcc/build-$ARCH/lib" \
        ..

    make -j$(nproc)
    make install
    cd ../..

    # Copy to output directory
    echo ""
    echo "Installing to $OUTPUT_DIR/lib/$ARCH..."
    mkdir -p "$OUTPUT_DIR/lib/$ARCH"
    mkdir -p "$OUTPUT_DIR/include"

    # Copy libraries
    cp "$WORK_DIR/install-$ARCH/lib/libnng.a" "$OUTPUT_DIR/lib/$ARCH/libnng.a"
    cp "$WORK_DIR/install-$ARCH/lib/libflatccrt.a" "$OUTPUT_DIR/lib/$ARCH/libflatccrt.a"

    # Copy headers (only once)
    if [ "$ARCH" = "x64" ]; then
        cp -r "$WORK_DIR/install-$ARCH/include/nng" "$OUTPUT_DIR/include/" 2>/dev/null || true
        cp -r "$WORK_DIR/install-$ARCH/include/flatcc" "$OUTPUT_DIR/include/" 2>/dev/null || true
    fi

    echo "✓ $ARCH build complete"
}

# Build both architectures
# IMPORTANT: Build x86 first, then x64
# flatcc outputs to a shared directory, so building x64 last ensures it's correct
build_arch "x86"
build_arch "x64"

# Create README
cat > "$OUTPUT_DIR/README.txt" << 'EOF'
PluQ Libraries for MinGW (UCRT)
================================

These libraries were built with LLVM-MinGW which has UCRT support.

Contents:
- nng 1.11 (stable, static library)
- flatcc runtime (static library)

Directory structure:
lib/x86/     - 32-bit libraries (libnng.a, libflatccrt.a)
lib/x64/     - 64-bit libraries (libnng.a, libflatccrt.a)
include/     - Headers (nng/, flatcc/)

Usage with MinGW Makefiles:
CFLAGS += -I../Windows/pluq/include
LDFLAGS += -L../Windows/pluq/lib/x86  (or x64)
LIBS += -lnng -lflatccrt

These libraries are compatible with:
- LLVM-MinGW (UCRT)
- MSYS2 UCRT64 toolchain
- Any MinGW toolchain using UCRT

Note: The pluq directory contains both MinGW (.a) and MSVC (.lib) libraries
sharing the same headers, similar to other Windows dependencies like SDL2.

NOTE: nng 1.11 requires UCRT on Windows (same as nng 2.x).
EOF

echo ""
echo "=========================================="
echo "  Build Complete!"
echo "=========================================="
echo ""
echo "Output directory: $OUTPUT_DIR"
echo ""
echo "Files created:"
ls -lh "$OUTPUT_DIR/lib/x86/"
echo ""
ls -lh "$OUTPUT_DIR/lib/x64/"
echo ""
echo "To use these libraries, update Makefile.w32/w64:"
echo "  CFLAGS += -I../Windows/pluq/include"
echo "  LDFLAGS += -L../Windows/pluq/lib/x86 (or x64)"
echo "  PLUQ_LIBS = -lnng -lflatccrt"
