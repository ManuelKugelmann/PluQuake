PluQ Libraries for MinGW (UCRT)
================================

These libraries were built with LLVM-MinGW which has UCRT support.

Contents:
- nng 1.11 (stable, static library)
- flatcc runtime (static library)

Directory structure:
lib/x86/     - 32-bit libraries (libnng.a, libflatccrt.a)
lib/x64/     - 64-bit libraries (libnng.a, libflatccrt.a)
include/     - Headers (nng/, flatcc/)

Usage with MinGW Makefiles:
CFLAGS += -I../Windows/pluq/include
LDFLAGS += -L../Windows/pluq/lib/x86  (or x64)
LIBS += -lnng -lflatccrt

These libraries are compatible with:
- LLVM-MinGW (UCRT)
- MSYS2 UCRT64 toolchain
- Any MinGW toolchain using UCRT

Note: The pluq directory contains both MinGW (.a) and MSVC (.lib) libraries
sharing the same headers, similar to other Windows dependencies like SDL2.

NOTE: nng 1.11 requires UCRT on Windows (same as nng 2.x).
