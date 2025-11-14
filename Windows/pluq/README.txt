# PluQ Precompiled Libraries (Windows x86/x64)

This directory contains precompiled libraries for PluQ IPC on Windows.

## Versions
- nng: 1.11
- flatcc: 0.6.1

## Build Information
**Build Date**: 2025-11-12 20:36:20
**Platform**: Windows (x86, x64)
**Compilers**: Visual Studio 2022, LLVM-MinGW (UCRT)
**Build Type**: Release (static libraries)

## Contents

### lib/x86/
- nng.lib (MSVC) - nng v1.11 for Visual Studio (32-bit)
- flatccrt.lib (MSVC) - flatcc v0.6.1 runtime for Visual Studio (32-bit)
- libnng.a (MinGW) - nng v1.11 for MinGW/UCRT (32-bit)
- libflatccrt.a (MinGW) - flatcc v0.6.1 runtime for MinGW/UCRT (32-bit)

### lib/x64/
- nng.lib (MSVC) - nng v1.11 for Visual Studio (64-bit)
- flatccrt.lib (MSVC) - flatcc v0.6.1 runtime for Visual Studio (64-bit)
- libnng.a (MinGW) - nng v1.11 for MinGW/UCRT (64-bit)
- libflatccrt.a (MinGW) - flatcc v0.6.1 runtime for MinGW/UCRT (64-bit)

### include/
- nng/ - nng headers (v1.11)
- flatcc/ - flatcc headers (v0.6.1)

## Features

### nng v1.11
- Protocol support: REQ/REP, PUB/SUB, PUSH/PULL
- Transport: TCP/IP (localhost)
- Thread-safe
- Zero-copy message passing
- UCRT runtime (required for Windows)

### flatcc v0.6.1
- FlatBuffers serialization for C
- Runtime-only build (no compiler tools)
- Zero-copy deserialization
- Schema-based type safety

## Usage

### Visual Studio Projects
```
Additional Include Directories: $(ProjectDir)\..\Windows\pluq\include
Additional Library Directories: $(ProjectDir)\..\Windows\pluq\lib\x64 (or x86)
Additional Dependencies: nng.lib;flatccrt.lib
```

### MinGW Makefiles
```makefile
CFLAGS = -I../Windows/pluq/include
LDFLAGS = -L../Windows/pluq/lib/x64 (or x86)
LIBS = -lnng -lflatccrt -lws2_32 -lmswsock
```

## Rebuild

To rebuild these libraries:

**For MinGW (from Linux with cross-compilation):**
```bash
cd Windows/
./build-pluq-mingw.sh
```

**For Visual Studio (from Windows):**
```powershell
cd Windows/
.\build-pluq-libs.ps1
```

## Sources
- nng: https://github.com/nanomsg/nng
- flatcc: https://github.com/dvidelabs/flatcc

## Notes

- Both MSVC and MinGW libraries share the same headers
- UCRT runtime is required (LLVM-MinGW, MSYS2 UCRT64, or MSVC)
- Libraries are statically linked
- Thread-safe for concurrent use

**NOTE**: Current prebuilt libraries were built with flatcc master branch.
To rebuild with flatcc v0.6.1, run the build scripts above.
