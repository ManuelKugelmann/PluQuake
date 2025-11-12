# PluQ Dependencies for Windows

This directory contains nng 1.11 and flatcc headers and libraries for Windows.
Similar to SDL2, this directory contains libraries for both MSVC and MinGW.

## Versions
- nng: 1.11 (stable)
- flatcc: master branch

## Build Information
- MSVC: Built with Visual Studio 2022 (via build-pluq-libs.ps1)
- MinGW: Built with LLVM-MinGW UCRT (via build-pluq-mingw.sh)
- Configuration: Release
- Architectures: x86, x64

## Contents
- include/nng/     - nng 1.11 headers (with protocol subdirectories)
- include/flatcc/  - flatcc headers
- lib/x86/         - 32-bit libraries (MSVC: nng.lib, flatccrt.lib / MinGW: libnng.a, libflatccrt.a)
- lib/x64/         - 64-bit libraries (MSVC: nng.lib, flatccrt.lib / MinGW: libnng.a, libflatccrt.a)

## Library Format
Both MSVC and MinGW libraries share the same headers and directory structure:
- MSVC: nng.lib, flatccrt.lib (.lib format)
- MinGW: libnng.a, libflatccrt.a (.a format)

This follows the same pattern as Windows/SDL2 and Windows/codecs.

## Rebuild
MSVC libraries (on Windows):
  .\build-pluq-libs.ps1

MinGW libraries (on Linux):
  ./build-pluq-mingw.sh

MSVC libraries are automatically built by the windows_ci.yml workflow.

## Usage
Makefiles automatically detect the correct library format based on the toolchain.

## Sources
- nng: https://github.com/nanomsg/nng
- flatcc: https://github.com/dvidelabs/flatcc

Updated: 2025-11-12 (nng 1.11 migration, unified structure)
