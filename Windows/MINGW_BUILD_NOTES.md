# MinGW Build Notes for PluQ

## Current Status

MinGW cross-compilation builds (Makefile.w32/w64) currently use **stub implementations** for PluQ functions. This means the executables compile successfully but PluQ IPC functionality is disabled.

## Why No PluQ Support in MinGW?

The prebuilt libraries in `Windows/pluq/lib/` are compiled with **Microsoft Visual Studio (MSVC)** and are incompatible with MinGW's GCC linker due to:

1. **Different name mangling**: MSVC uses `_imp__nng_*` import naming
2. **Different runtime libraries**: MSVC uses `__security_cookie`, `@__security_check_cookie@4`
3. **Different helper functions**: MSVC uses `_allmul` for 64-bit multiply in 32-bit code

Additionally, **nng 2.0 requires Universal C Runtime (UCRT)** which is not available in standard MinGW-w64. The nng build explicitly rejects "legacy MinGW environments".

## Solutions

### Option 1: Use Visual Studio (Recommended)

The Visual Studio project (`Windows/VisualStudio/ironwail.vcxproj`) has full PluQ support with prebuilt libraries.

### Option 2: Build with MSYS2 MinGW-UCRT

MSYS2 provides MinGW-w64 with UCRT support:

```bash
# In MSYS2 UCRT64 shell
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake
cd Quake
./build-pluq-mingw.sh x64
```

### Option 3: Accept Stub Implementations

MinGW cross-compilation builds work fine without PluQ. The stub implementations ensure compilation succeeds.

## Future Work

- Build MinGW-compatible nng/flatcc libraries with UCRT toolchain
- Add these to `Windows/pluq-mingw/` directory
- Update Makefiles to detect and use MinGW libs when available

## Technical Details

The stub implementations are in `Quake/pluq.c` guarded by `#ifdef NO_PLUQ_LIBS`:

- `PluQ_Init()` - does nothing
- `PluQ_Shutdown()` - does nothing
- `PluQ_BroadcastWorldState()` - does nothing
- `PluQ_ProcessInputCommands()` - does nothing
- `PluQ_Move()` - does nothing
- `PluQ_ApplyViewAngles()` - does nothing

These allow the game to compile and run normally, just without IPC capabilities.
