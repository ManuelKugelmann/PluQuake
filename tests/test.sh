#!/bin/bash
# PluQ IPC Test Suite

echo "=== PluQ IPC Tests ==="
echo ""

# Gameplay Channel Test
echo "1. Testing Gameplay Channel (PUB/SUB)..."
./test-backend-simulator &
BACKEND_PID=$!
sleep 1
./test-monitor-simple
wait $BACKEND_PID
echo ""

# Input Channel Test  
echo "2. Testing Input Channel (PUSH/PULL)..."
timeout 5 ./test-input-receiver &
sleep 1
./test-command "move forward"
./test-command "turn left"
./test-command "fire"
sleep 1
pkill -f test-input-receiver 2>/dev/null
echo ""

echo "=== Tests Complete ==="
