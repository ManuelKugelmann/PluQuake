# PluQ - IPC-based Remote Rendering for Quake

PluQ (Pluggable Quake) enables decoupled frontend/backend architecture for Quake using IPC channels.

## Architecture

**Three-channel IPC system:**
- **Resources Channel** (REQ/REP): On-demand resource requests
- **Gameplay Channel** (PUB/SUB): Backend broadcasts game state to frontend  
- **Input Channel** (PUSH/PULL): Frontend sends input to backend

**Transport:** nng v1.11 with TCP (localhost)
**Serialization:** FlatBuffers (optimized to match Quake's precision)

## Components

- `ironwail` - Standard Quake backend
- `ironwail-pluq-frontend` - Remote rendering frontend
- `ironwail-pluq-test-frontend` - Minimal test frontend

## Building

### Dependencies (auto-downloaded)
```bash
cd Quake
./download-dependencies.sh
```

Builds: SDL2, nng, flatcc

### Linux/macOS
```bash
cd Quake
make
```

### Windows (MinGW cross-compile)
```bash
cd Quake
./build-pluq-mingw.sh
```

## Testing

### Gameplay Channel Test
```bash
cd pluq-deployment
./test-backend-simulator &
./test-monitor-simple
```

**Expected:** 100% frame delivery (110/110 frames)

### Input Channel Test
```bash
./test-input-receiver &
./test-command "test command"
```

## Protocol Optimization

**FlatBuffers schema optimized to match Quake's precision:**
- Vec3Coord: `int16[3]` (6 bytes, was 12) - 50% reduction
- Vec3Angle: `uint8[3]` (3 bytes, was 12) - 75% reduction  
- Entity.alpha: `ubyte` (1 byte, was 4) - 75% reduction
- Movement: `int16` (2 bytes, was 4) - 50% reduction

**Result:** ~20-30% bandwidth savings while maintaining Quake's precision.

## Schema

Located in `Quake/pluq.fbs` - compile with:
```bash
/home/user/PluQuake/Linux/pluq/bin/flatcc -a -o Quake/ Quake/pluq.fbs
```

## Status

✓ Gameplay channel: 100% frame delivery
✓ Input channel: Working
✓ Schema optimized for bandwidth
✓ Prebuilt libraries for Linux/macOS/Windows
○ Resources channel: Not implemented (optional)

See `IPC_IMPLEMENTATION_STATUS.md` for detailed status.
