#!/bin/bash
# Integration test: Real headless backend + headless test frontend
# Tests actual IPC communication between engine components

cd "$(dirname "$0")"

echo "=== PluQ Integration Test ==="
echo ""

# Check for binaries
BACKEND="../Quake/ironwail"
FRONTEND="../Quake/ironwail-pluq-test-frontend"

if [ ! -f "$BACKEND" ]; then
    echo "ERROR: Backend not found at $BACKEND"
    echo "Build with: cd Quake && make -j4"
    exit 1
fi

if [ ! -f "$FRONTEND" ]; then
    echo "ERROR: Test frontend not found at $FRONTEND"
    echo "Build with: cd Quake && make -f Makefile.pluq_test_frontend -j4"
    exit 1
fi

# Download shareware if needed
if [ ! -f "id1/pak0.pak" ]; then
    echo "Downloading Quake shareware..."
    ./download-shareware.sh || exit 1
fi

# Set library path if using local dependencies
if [ -d "../Quake/dependencies/lib" ]; then
    export LD_LIBRARY_PATH="../Quake/dependencies/lib:$LD_LIBRARY_PATH"
fi

# Temp file for frontend output
OUTPUT_FILE=$(mktemp)
trap "rm -f $OUTPUT_FILE; kill $BACKEND_PID $FRONTEND_PID 2>/dev/null" EXIT

echo "Starting headless backend..."
$BACKEND -basedir . -headless -pluq +map start > /dev/null 2>&1 &
BACKEND_PID=$!

sleep 3

if ! kill -0 $BACKEND_PID 2>/dev/null; then
    echo "FAIL: Backend failed to start"
    exit 1
fi
echo "Backend running (PID $BACKEND_PID)"

echo "Starting headless test frontend (5 second test)..."
timeout 5 $FRONTEND -basedir . > "$OUTPUT_FILE" 2>&1 &
FRONTEND_PID=$!

sleep 5

# Count received frames
FRAME_COUNT=$(grep -c "World state" "$OUTPUT_FILE" 2>/dev/null || echo "0")

echo ""
echo "=== Results ==="
echo "Frames received: $FRAME_COUNT"

if [ "$FRAME_COUNT" -gt 0 ]; then
    echo "PASS: IPC communication working"
    exit 0
else
    echo "FAIL: No frames received"
    echo ""
    echo "Frontend output:"
    cat "$OUTPUT_FILE"
    exit 1
fi
