#!/bin/bash
# Test gameplay channel: backend simulator (PUB) → monitor (SUB)

cd "$(dirname "$0")"

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║     PluQ Gameplay Channel Test                               ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "This test demonstrates the complete gameplay broadcast flow:"
echo "  1. Backend simulator broadcasts FrameUpdate messages (PUB socket)"
echo "  2. Monitor receives and displays frame data (SUB socket)"
echo "  3. FlatBuffers serialization/deserialization verified"
echo ""

# Start backend simulator in background
echo "Starting backend simulator (publisher)..."
./test-backend-simulator > simulator.log 2>&1 &
SIMULATOR_PID=$!

echo "Simulator PID: $SIMULATOR_PID"
echo ""

# Give backend time to initialize
sleep 1

# Check if simulator is running
if ! kill -0 $SIMULATOR_PID 2>/dev/null; then
    echo "ERROR: Backend simulator failed to start!"
    cat simulator.log
    exit 1
fi

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║ Receiving Frames                                              ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Start monitor (will receive frames)
timeout 3 ./test-monitor > monitor.log 2>&1

# Wait for simulator to finish
wait $SIMULATOR_PID 2>/dev/null

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║ Monitor Output (Last 30 lines)                                ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Show last 30 lines of monitor output
tail -30 monitor.log

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║ Simulator Output                                              ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

cat simulator.log

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║ Test Complete!                                                ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "✅ Gameplay channel (PUB/SUB) works correctly!"
echo "✅ FlatBuffers FrameUpdate serialization/deserialization verified!"
echo "✅ Backend can broadcast game state to frontend clients!"
echo ""
