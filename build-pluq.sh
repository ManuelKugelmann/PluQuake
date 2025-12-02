#!/bin/bash
# Build script for PluQ backend and test frontend
# Usage: ./build-pluq.sh [clean|backend|frontend|all]

set -e

# Get absolute path to script directory (before any cd)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
QUAKE_DIR="$SCRIPT_DIR/Quake"
TESTS_DIR="$SCRIPT_DIR/tests"

cd "$QUAKE_DIR"

# Detect HOST_OS if not set
if [ -z "$HOST_OS" ]; then
    case "$(uname -s)" in
        Linux*) HOST_OS=linux ;;
        Darwin*) HOST_OS=darwin ;;
        MINGW*|MSYS*|CYGWIN*) HOST_OS=win64 ;;
        *) HOST_OS=linux ;;
    esac
fi
export HOST_OS

clean_objects() {
    echo "Cleaning object files..."
    rm -f *.o *.d
}

copy_to_tests() {
    local binary="$1"
    if [ -f "$binary" ]; then
        echo "Copying $binary to tests folder..."
        cp "$binary" "$TESTS_DIR/"
        echo "  -> $TESTS_DIR/$binary"
    fi
}

build_backend() {
    echo "Building PluQ backend (ironwail)..."
    make -j4
    echo "Backend built: ironwail"
    copy_to_tests "ironwail"
}

build_frontend() {
    echo "Building PluQ test frontend..."
    make -f Makefile.pluq_test_frontend -j4
    echo "Frontend built: ironwail-pluq-test-frontend"
    copy_to_tests "ironwail-pluq-test-frontend"
}

case "${1:-all}" in
    clean)
        clean_objects
        ;;
    backend)
        clean_objects
        build_backend
        ;;
    frontend)
        clean_objects
        build_frontend
        ;;
    all)
        clean_objects
        build_backend
        clean_objects
        build_frontend
        ;;
    *)
        echo "Usage: $0 [clean|backend|frontend|all]"
        exit 1
        ;;
esac

echo "Done!"
