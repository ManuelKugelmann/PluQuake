#!/bin/bash
# PLQ IPC Integration Test Script
# Tests communication between backend and frontend

set -e
cd "$(dirname "$0")"

# Configuration
BACKEND="../Quake/ironwail"
FRONTEND="../Quake/ironwail-pluq-test-frontend"
BACKEND_TIMEOUT=15
FRONTEND_TIMEOUT=6
STARTUP_DELAY=4

# Cleanup function
cleanup() {
    pkill -9 -f ironwail 2>/dev/null || true
    rm -f /tmp/plq_backend.log /tmp/plq_frontend.log
}

trap cleanup EXIT

echo "=== PLQ IPC Integration Test ==="
echo ""

# Check binaries
if [ ! -f "$BACKEND" ]; then
    echo "ERROR: Backend not found at $BACKEND"
    echo "Build with: cd Quake && HOST_OS=linux make -j4"
    exit 1
fi

if [ ! -f "$FRONTEND" ]; then
    echo "ERROR: Frontend not found at $FRONTEND"
    echo "Build with: cd Quake && HOST_OS=linux make -f Makefile.pluq_test_frontend -j4"
    exit 1
fi

# Check shareware
if [ ! -f "id1/pak0.pak" ]; then
    echo "Downloading Quake shareware..."
    ./download-shareware.sh || exit 1
fi

# Kill any existing instances
cleanup

# Start backend
echo "Starting backend..."
timeout $BACKEND_TIMEOUT $BACKEND -basedir . -headless -pluq -conout > /tmp/plq_backend.log 2>&1 &
BACKEND_PID=$!

sleep $STARTUP_DELAY

# Check backend started
if ! kill -0 $BACKEND_PID 2>/dev/null; then
    echo "FAIL: Backend failed to start"
    echo ""
    echo "=== Backend log ==="
    cat /tmp/plq_backend.log
    exit 1
fi
echo "Backend running (PID $BACKEND_PID)"

# Start frontend
echo "Starting frontend for $FRONTEND_TIMEOUT seconds..."
timeout $FRONTEND_TIMEOUT $FRONTEND -basedir . > /tmp/plq_frontend.log 2>&1 || true

# Results
echo ""
echo "=== Results ==="
echo ""

echo "--- Backend log (last 30 lines) ---"
tail -30 /tmp/plq_backend.log

echo ""
echo "--- Frontend log ---"
cat /tmp/plq_frontend.log | head -40

echo ""
echo "--- Statistics ---"
FRAMES_SENT=$(grep -c "Broadcasting frame" /tmp/plq_backend.log 2>/dev/null || echo "0")
FRAMES_RECV=$(grep -c "Received frame" /tmp/plq_frontend.log 2>/dev/null || echo "0")
echo "Frames sent by backend: $FRAMES_SENT"
echo "Frames received by frontend: $FRAMES_RECV"

if [ "$FRAMES_RECV" -gt "0" ]; then
    echo ""
    echo "PASS: IPC communication working!"
    exit 0
else
    echo ""
    echo "FAIL: No frames received by frontend"
    exit 1
fi
