# PluQ Precompiled Libraries (Linux x86_64)

This directory contains precompiled libraries for PluQ IPC on Linux.

## Versions
- nng: 1.11
- flatcc: 0.6.1

## Build Information
**Build Date**: 2025-11-14
**Platform**: Linux x86_64
**Compiler**: gcc 13
**Build Type**: Release (static libraries)

## Contents

### lib/
- libnng.a (1.2M) - nng v1.11 (nanomsg-next-generation) static library
- libflatccrt.a (199K) - flatcc v0.6.1 (FlatBuffers for C) runtime library
- cmake/ - CMake package configuration files

### include/
- nng/ - nng headers (v1.11)
- flatcc/ - flatcc headers (v0.6.1)

## Features

### nng v1.11
- Protocol support: REQ/REP, PUB/SUB, PUSH/PULL
- Transport: TCP/IP (localhost)
- Thread-safe
- Zero-copy message passing

### flatcc v0.6.1
- FlatBuffers serialization for C
- Runtime-only build (no compiler tools)
- Zero-copy deserialization
- Schema-based type safety

## Usage

### In Makefiles
```makefile
CFLAGS = -I../Linux/pluq/include
LDFLAGS = -L../Linux/pluq/lib -lnng -lflatccrt -lpthread
```

### Test Programs
All test programs in pluq-deployment/ use these prebuilt libraries.

## Rebuild

To rebuild these libraries, run:
```bash
cd Linux/
./build-pluq-libs.sh
```

## Sources
- nng: https://github.com/nanomsg/nng
- flatcc: https://github.com/dvidelabs/flatcc

## Verified

These libraries have been tested and verified with:
- ✅ Input channel (PUSH/PULL) - 100% success rate
- ✅ Gameplay channel (PUB/SUB) - 98% reception rate
- ✅ FlatBuffers serialization/deserialization
- ✅ Real-time performance at 60 FPS

See IPC_CHANNEL_TEST_REPORT.md for full test results.
