#!/bin/bash
# Build nng and flatcc for MinGW with UCRT support
# Uses LLVM-MinGW toolchain which supports UCRT

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="$SCRIPT_DIR/build-pluq-mingw-ucrt"
OUTPUT_DIR="$SCRIPT_DIR/pluq-mingw"

# LLVM-MinGW version and download URL
LLVM_MINGW_VERSION="20251104"
LLVM_MINGW_ARCH="x86_64"  # We'll build both x86 and x64 libraries with this

echo "=========================================="
echo "  Building PluQ for MinGW with UCRT"
echo "=========================================="
echo ""
echo "This uses LLVM-MinGW which has UCRT support"
echo "required by nng 2.0"
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
set(CMAKE_FIND_ROOT_PATH $TOOLCHAIN_DIR/$TARGET_PREFIX)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF

    # Build nng
    echo ""
    echo "Step 1/2: Building nng..."

    if [ ! -d "nng" ]; then
        git clone --depth 1 -b v2.0.0-alpha.6 https://github.com/nanomsg/nng.git

        # Patch nng to fix type mismatches with LLVM-MinGW
        cd nng
        # Fix nni_socket_pair return type (change 'int' to 'nng_err')
        sed -i '/^int$/{ N; s/^int\nnni_socket_pair(int fds\[2\])$/nng_err\nnni_socket_pair(int fds[2])/; }' src/platform/windows/win_socketpair.c
        cd ..
    fi

    mkdir -p "nng/build-$ARCH"
    cd "nng/build-$ARCH"

    cmake \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_FLAGS="-Wno-error -Wno-implicit-function-declaration -Wno-int-conversion -Wno-incompatible-function-pointer-types" \
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
        ..

    make -j$(nproc)
    make install
    cd ../..

    # Copy to output directory
    echo ""
    echo "Installing to $OUTPUT_DIR/lib/$ARCH..."
    mkdir -p "$OUTPUT_DIR/lib/$ARCH"
    mkdir -p "$OUTPUT_DIR/include"

    # Copy libraries (convert .a to .lib for consistency with MSVC naming)
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
build_arch "x64"
build_arch "x86"

# Create README
cat > "$OUTPUT_DIR/README.txt" << 'EOF'
PluQ Libraries for MinGW (UCRT)
================================

These libraries were built with LLVM-MinGW which has UCRT support.

Contents:
- nng 2.0.0-alpha.6 (static library)
- flatcc runtime (static library)

Directory structure:
lib/x86/     - 32-bit libraries (libnng.a, libflatccrt.a)
lib/x64/     - 64-bit libraries (libnng.a, libflatccrt.a)
include/     - Headers (nng/, flatcc/)

Usage with MinGW Makefiles:
CFLAGS += -I../Windows/pluq-mingw/include
LDFLAGS += -L../Windows/pluq-mingw/lib/x86  (or x64)
LIBS += -lnng -lflatccrt

These libraries are compatible with:
- LLVM-MinGW (UCRT)
- MSYS2 UCRT64 toolchain
- Any MinGW toolchain using UCRT

NOT compatible with:
- Standard MinGW-w64 (uses MSVCRT, not UCRT)
- MSVC (use Windows/pluq/ instead)
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
echo "  1. Remove -DNO_PLUQ_LIBS"
echo "  2. Add: CFLAGS += -I../Windows/pluq-mingw/include"
echo "  3. Add: LDFLAGS += -L../Windows/pluq-mingw/lib/x86"
echo "  4. Add: PLUQ_LIBS = -lnng -lflatccrt"
