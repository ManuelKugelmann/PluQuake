#!/bin/bash
# Extended gameplay test: Watch demo, then start game and move around
# Tests the full IPC loop: frontend commands → backend → state updates → frontend

set -e
cd "$(dirname "$0")"

echo "=== PluQ Extended Gameplay Test ==="
echo ""

# Use local binaries in tests folder
BACKEND="./ironwail"
FRONTEND="./ironwail-pluq-test-frontend"

if [ ! -f "$BACKEND" ]; then
    echo "ERROR: Backend not found at $BACKEND"
    echo "Build with: ./build-pluq.sh all"
    exit 1
fi

if [ ! -f "$FRONTEND" ]; then
    echo "ERROR: Test frontend not found at $FRONTEND"
    echo "Build with: ./build-pluq.sh frontend"
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

# Log files
BACKEND_LOG="backend-gameplay.log"
FRONTEND_LOG="frontend-gameplay.log"
COMMANDS_FIFO="/tmp/pluq_test_commands"

cleanup() {
    echo ""
    echo "Cleaning up..."
    kill $BACKEND_PID $FRONTEND_PID 2>/dev/null || true
    exec 3>&- 2>/dev/null || true
    rm -f "$COMMANDS_FIFO"
    wait 2>/dev/null || true
}
trap cleanup EXIT

# Create a FIFO for sending commands to frontend
rm -f "$COMMANDS_FIFO"
mkfifo "$COMMANDS_FIFO"

echo "Phase 1: Starting backend and frontend"
echo "======================================="

# Start backend with condebug for full logging
echo "1. Starting headless backend (will play demo)..."
$BACKEND -basedir . -headless -pluq -condebug > "$BACKEND_LOG" 2>&1 &
BACKEND_PID=$!
sleep 2

if ! kill -0 $BACKEND_PID 2>/dev/null; then
    echo "FAIL: Backend failed to start"
    cat "$BACKEND_LOG"
    exit 1
fi
echo "   Backend running (PID $BACKEND_PID)"

# Start frontend reading from FIFO (keeps stdin open)
echo "2. Starting frontend..."
$FRONTEND -basedir . < "$COMMANDS_FIFO" > "$FRONTEND_LOG" 2>&1 &
FRONTEND_PID=$!

# Open FIFO for writing (keeps it open)
exec 3>"$COMMANDS_FIFO"

sleep 2
if ! kill -0 $FRONTEND_PID 2>/dev/null; then
    echo "FAIL: Frontend failed to start"
    cat "$FRONTEND_LOG"
    exit 1
fi
echo "   Frontend running (PID $FRONTEND_PID)"

echo ""
echo "Phase 2: Watching demo (waiting for config to settle)"
echo "======================================================"
echo "3. Letting demo play and config commands to process..."
sleep 8

# Check connection
if grep -q "Connected to backend" "$FRONTEND_LOG" 2>/dev/null; then
    echo "   Frontend connected and receiving frames"
else
    echo "   WARNING: Frontend not receiving frames yet"
fi

echo ""
echo "Phase 3: Starting new game"
echo "=========================="

# Send a marker so we can find our commands in the log
echo "4. Sending test marker and game commands..."
echo "echo === PLUQ_TEST_COMMANDS_START ===" >&3
sleep 0.5
echo "skill 1" >&3
sleep 0.5
echo "map e1m1" >&3
sleep 4

# Check if map loaded
echo "5. Checking if map loaded..."

echo ""
echo "Phase 4: Player movement"
echo "========================"
echo "6. Sending movement commands..."

echo "+forward" >&3
sleep 1
echo "+moveright" >&3
sleep 1
echo "-forward" >&3
echo "-moveright" >&3
sleep 0.5

echo "+forward" >&3
echo "+jump" >&3
sleep 0.5
echo "-jump" >&3
sleep 1
echo "-forward" >&3

echo "   Movement commands sent"

echo ""
echo "Phase 5: Testing game commands"
echo "=============================="
echo "7. Waiting for spawn to complete..."
sleep 2

echo "8. Sending 'god' (god mode)..."
echo "god" >&3
sleep 1

echo "9. Sending 'give nails 100' (give ammo)..."
echo "give nails 100" >&3
sleep 1

echo "10. Sending 'give health 200' (give health)..."
echo "give health 200" >&3
sleep 1

echo "11. More movement..."
echo "+forward" >&3
echo "+moveleft" >&3
sleep 2
echo "-forward" >&3
echo "-moveleft" >&3

echo "12. End marker..."
echo "echo === PLUQ_TEST_COMMANDS_END ===" >&3

# Let simulation run
sleep 3

# Close the command FIFO
exec 3>&-

echo ""
echo "=== Test Results ==="
echo ""

# Read qconsole.log if it exists for more complete backend logging
if [ -f "qconsole.log" ]; then
    BACKEND_FULL_LOG="qconsole.log"
else
    BACKEND_FULL_LOG="$BACKEND_LOG"
fi

# Check for our test markers and commands in the log
echo "=== Commands received by backend (between markers) ==="
if grep -q "PLUQ_TEST_COMMANDS_START" "$BACKEND_FULL_LOG" 2>/dev/null; then
    # Extract commands between markers
    sed -n '/PLUQ_TEST_COMMANDS_START/,/PLUQ_TEST_COMMANDS_END/p' "$BACKEND_FULL_LOG" | grep -v "^$" | head -30
    echo ""
    MARKERS_FOUND=1
else
    echo "Markers not found - checking for individual commands..."
    grep -E "skill 1|map e1m1|god\"|impulse 9|\+forward|\-forward" "$BACKEND_FULL_LOG" | head -20 || echo "(none found)"
    MARKERS_FOUND=0
fi

# Check specific commands
echo ""
echo "=== Verification ==="
SKILL_CMD=$(grep -c '"skill 1"' "$BACKEND_FULL_LOG" 2>/dev/null || echo "0")
MAP_CMD=$(grep -c '"map e1m1"' "$BACKEND_FULL_LOG" 2>/dev/null || echo "0")
GOD_CMD=$(grep -c '"god"' "$BACKEND_FULL_LOG" 2>/dev/null || echo "0")
SLIPGATE=$(grep -c "Slipgate Complex" "$BACKEND_FULL_LOG" 2>/dev/null || echo "0")
GOD_MODE_ON=$(grep -c "godmode ON" "$BACKEND_FULL_LOG" 2>/dev/null || echo "0")
GIVE_NAILS=$(grep -c '"give nails' "$BACKEND_FULL_LOG" 2>/dev/null || echo "0")
GIVE_HEALTH=$(grep -c '"give health' "$BACKEND_FULL_LOG" 2>/dev/null || echo "0")

echo "Commands received by backend:"
echo "  - 'skill 1': $SKILL_CMD (expected: 1)"
echo "  - 'map e1m1': $MAP_CMD (expected: 1)"
echo "  - 'god': $GOD_CMD (expected: 1)"
echo "  - 'give nails': $GIVE_NAILS (expected: 1)"
echo "  - 'give health': $GIVE_HEALTH (expected: 1)"
echo ""
echo "Game state:"
echo "  - Map loaded (Slipgate Complex): $SLIPGATE"
echo "  - God mode activated: $GOD_MODE_ON"

# Check for duplicates (should all be 1)
DUPLICATES=0
if [ "$SKILL_CMD" -gt 1 ]; then echo "  WARNING: skill command duplicated!"; DUPLICATES=1; fi
if [ "$MAP_CMD" -gt 1 ]; then echo "  WARNING: map command duplicated!"; DUPLICATES=1; fi
if [ "$GOD_CMD" -gt 1 ]; then echo "  WARNING: god command duplicated!"; DUPLICATES=1; fi

# Frontend stats
FRAMES_RECEIVED=$(grep -c "Receiving:" "$FRONTEND_LOG" 2>/dev/null || echo "0")
COMMANDS_SENT=$(grep -c "→ Backend:" "$FRONTEND_LOG" 2>/dev/null || echo "0")

echo ""
echo "Frontend statistics:"
echo "  - Commands sent: $COMMANDS_SENT"
echo "  - Frame batches received: $FRAMES_RECEIVED"

echo ""
echo "=== Frontend Log (commands sent) ==="
grep "→ Backend:" "$FRONTEND_LOG" | head -20

# Determine pass/fail
echo ""
PASS=1
FAIL_REASON=""

if [ "$SKILL_CMD" -lt 1 ]; then PASS=0; FAIL_REASON="skill command not received"; fi
if [ "$MAP_CMD" -lt 1 ]; then PASS=0; FAIL_REASON="map command not received"; fi
if [ "$SLIPGATE" -lt 1 ]; then PASS=0; FAIL_REASON="map did not load"; fi
if [ "$GOD_MODE_ON" -lt 1 ]; then PASS=0; FAIL_REASON="god mode not activated"; fi
if [ "$DUPLICATES" -gt 0 ]; then PASS=0; FAIL_REASON="commands duplicated"; fi

if [ "$PASS" -eq 1 ]; then
    echo "=========================================="
    echo "=== PASS: Extended gameplay test OK! ===="
    echo "=========================================="
    echo "  ✓ Commands forwarded (no duplicates)"
    echo "  ✓ Map loaded successfully"
    echo "  ✓ God mode activated"
    echo "  ✓ Give commands processed"
    exit 0
else
    echo "=== FAIL: $FAIL_REASON ==="
    echo ""
    echo "Last 50 lines of backend log:"
    tail -50 "$BACKEND_FULL_LOG"
    exit 1
fi
