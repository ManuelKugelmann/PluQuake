PluQ Libraries for MinGW (UCRT)
================================

These libraries were built with LLVM-MinGW which has UCRT support.

Contents:
- nng 2.0.0-alpha.6 (static library)
- flatcc runtime (static library)

Directory structure:
lib/x86/     - 32-bit libraries (libnng.a, libflatccrt.a)
lib/x64/     - 64-bit libraries (libnng.a, libflatccrt.a)
include/     - Headers (nng/, flatcc/)

Usage with MinGW Makefiles:
CFLAGS += -I../Windows/pluq-mingw/include
LDFLAGS += -L../Windows/pluq-mingw/lib/x86  (or x64)
LIBS += -lnng -lflatccrt

These libraries are compatible with:
- LLVM-MinGW (UCRT)
- MSYS2 UCRT64 toolchain
- Any MinGW toolchain using UCRT

NOT compatible with:
- Standard MinGW-w64 (uses MSVCRT, not UCRT)
- MSVC (use Windows/pluq/ instead)
