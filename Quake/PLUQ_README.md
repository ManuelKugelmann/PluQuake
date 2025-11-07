# PluQ IPC Integration for QuakeSpasm

This directory contains the integrated PluQ IPC system for broadcasting game state to external frontends (Unity, Unreal Engine, etc.).

## Building with PluQ Support

### Prerequisites

1. **NNG (Nanomsg Next Generation)** - Messaging library
2. **FlatCC** - FlatBuffers C compiler and runtime

### Installing Dependencies

#### Ubuntu/Debian
```bash
# Install NNG
git clone https://github.com/nanomsg/nng.git
cd nng
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
sudo cmake --install .

# Install FlatCC
git clone https://github.com/dvidelabs/flatcc.git
cd flatcc
scripts/build.sh
sudo cp -r bin/Release/* /usr/local/bin/
sudo cp -r lib/Release/* /usr/local/lib/
sudo cp -r include/* /usr/local/include/
```

#### macOS
```bash
# Install NNG
brew install nng

# Install FlatCC
brew install flatcc
```

### Generate FlatBuffers Code

Before building QuakeSpasm with PluQ, generate the FlatBuffers code:

```bash
cd Quake
./generate_all.sh
```

This will create `generated/pluq_builder.h` and related files.

### Build QuakeSpasm

```bash
cd Quake
make clean
make USE_PLUQ=1
```

To build without PluQ:
```bash
make USE_PLUQ=0
```

## Using PluQ

### Console Commands

- `pluq_init` - Initialize PluQ with current settings
- `pluq_shutdown` - Shutdown PluQ
- `pluq_stats` - Show performance statistics

### Console Variables

- `pluq_enable` - Enable/disable PluQ (0 or 1)
- `pluq_mode` - Mode: "backend" or "frontend"
- `pluq_transport` - Transport: "tcp", "ipc", or "ws"
- `pluq_address` - Address to bind/connect to (default: "tcp://0.0.0.0:5555")

### Example Usage

#### As Backend (Broadcasting Game State)

```
pluq_enable 1
pluq_mode backend
pluq_transport tcp
pluq_address tcp://0.0.0.0:5555
pluq_init
```

Now your QuakeSpasm instance will broadcast game state on port 5555.

#### As Frontend (Receiving Game State)

```
pluq_enable 1
pluq_mode frontend
pluq_transport tcp
pluq_address tcp://localhost:5555
pluq_init
```

## Architecture

### Files

- `pluq.h` - PluQ C API header
- `pluq.c` - PluQ C implementation using NNG + FlatBuffers
- `pluq.fbs` - FlatBuffers schema
- `generated/` - Generated FlatBuffers code

### Protocol

PluQ uses FlatBuffers for zero-copy serialization and NNG for transport. Each frame contains:

- Player state (position, angles, velocity, health, armor, weapon, ammo)
- Game state (paused, in_game, intermission, mapname, time, gravity, maxspeed)
- Entities (origin, angles, model_id, skin, frame, effects, alpha, scale)
- Dynamic lights (origin, radius, color, decay)

### Transport Options

- **TCP**: Network transport, works across machines (default: port 5555)
- **IPC**: Unix domain socket, fastest for same machine (e.g., ipc:///tmp/pluq)
- **WebSocket**: For browser-based frontends (e.g., ws://0.0.0.0:8080)

## Frontend Examples

See the PluQ repository for complete examples:

- C: `/home/user/pluq/c/examples/`
- C++: `/home/user/pluq/cpp/examples/`
- C#/Unity: `/home/user/pluq/unity/`
- C++/Unreal: `/home/user/pluq/unreal/`

## Troubleshooting

### Build Errors

**"nng/nng.h: No such file or directory"**
- Install NNG library (see Prerequisites above)
- Make sure NNG headers are in `/usr/local/include/` or `/usr/include/`

**"flatcc/flatcc_builder.h: No such file or directory"**
- Install FlatCC (see Prerequisites above)
- Run `./generate_all.sh` to generate FlatBuffers code

**"cannot find -lnng"**
- Install NNG library
- Make sure `libnng.so` is in `/usr/local/lib/` or `/usr/lib/`
- Run `sudo ldconfig` after installing

### Runtime Errors

**"PluQ initialization failed: Address already in use"**
- Another process is using the port
- Change `pluq_address` to use a different port

**"PluQ initialization failed: Permission denied"**
- Check file permissions for IPC socket
- Try using a different IPC path in `/tmp/`

## Performance

- Backend overhead: < 1ms per frame @ 72 FPS
- Network latency (TCP): 1-5ms on localhost
- IPC latency: 10-50µs (microseconds)
- Frame size: ~5-50KB depending on entity count

## License

PluQ is licensed under the MIT License.
QuakeSpasm is licensed under the GNU General Public License v2.
