# MinGW Build Notes for PluQ

## Current Status (Updated 2025-11-12)

MinGW cross-compilation builds now have **full PluQ support** with nng 1.11 and flatcc libraries built using LLVM-MinGW with UCRT support.

## Unified Library Structure

PluQ libraries for both MSVC and MinGW are now in `Windows/pluq/`:

```
Windows/pluq/
  include/           - Shared headers (nng 1.11 + flatcc)
  lib/x64/
    libnng.a         - MinGW 64-bit
    libflatccrt.a    - MinGW 64-bit
    nng.lib          - MSVC 64-bit (built by CI)
    flatccrt.lib     - MSVC 64-bit (built by CI)
  lib/x86/
    libnng.a         - MinGW 32-bit
    libflatccrt.a    - MinGW 32-bit
    nng.lib          - MSVC 32-bit (built by CI)
    flatccrt.lib     - MSVC 32-bit (built by CI)
```

This mirrors the structure used by SDL2 and other Windows dependencies.

## Building with MinGW

### Cross-compilation from Linux

MinGW libraries are automatically built using LLVM-MinGW:

```bash
cd Windows
./build-pluq-mingw.sh
```

The script:
- Downloads LLVM-MinGW with UCRT support
- Builds nng 1.11 for both x86 and x64
- Builds flatcc runtime for both architectures
- Installs libraries to `Windows/pluq/lib/`

### Cross-compilation build scripts

```bash
cd Quake
./build_cross_win64-sdl2.sh  # 64-bit
./build_cross_win32-sdl2.sh  # 32-bit
```

These automatically use the libraries from `Windows/pluq/`.

## Why LLVM-MinGW?

**nng 1.11 requires Universal C Runtime (UCRT)** which is not available in standard MinGW-w64. LLVM-MinGW provides:

1. **UCRT support** - Required by nng 1.11
2. **Modern toolchain** - Clang/LLVM based
3. **Cross-platform builds** - Works on Linux for Windows targets

## Technical Details

The stub implementations are in `Quake/pluq.c` guarded by `#ifdef NO_PLUQ_LIBS`:

- `PluQ_Init()` - does nothing
- `PluQ_Shutdown()` - does nothing
- `PluQ_BroadcastWorldState()` - does nothing
- `PluQ_ProcessInputCommands()` - does nothing
- `PluQ_Move()` - does nothing
- `PluQ_ApplyViewAngles()` - does nothing

These allow the game to compile and run normally, just without IPC capabilities.
