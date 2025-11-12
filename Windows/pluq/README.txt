# PluQ Dependencies for Windows (MSVC)

This directory contains nng 1.11 and flatcc headers and libraries for Windows.

## Versions
- nng: 1.11 (stable)
- flatcc: master branch

## Build Information
- Built with: Visual Studio 2022 (via build-pluq-libs.ps1)
- Configuration: Release
- Architectures: x86, x64

## Contents
- include/nng/     - nng 1.11 headers (with protocol subdirectories)
- include/flatcc/  - flatcc headers
- lib/x86/         - 32-bit libraries (nng.lib, flatccrt.lib) - built by CI
- lib/x64/         - 64-bit libraries (nng.lib, flatccrt.lib) - built by CI

## Headers
The headers are shared with the MinGW build (pluq-mingw) since nng 1.11
headers are the same for both toolchains. Only the library format differs:
- MSVC: nng.lib, flatccrt.lib
- MinGW: libnng.a, libflatccrt.a

## Rebuild
To rebuild these libraries on Windows, run:
  .\build-pluq-libs.ps1

The libraries are automatically built by the windows_ci.yml workflow.

## Sources
- nng: https://github.com/nanomsg/nng
- flatcc: https://github.com/dvidelabs/flatcc

Headers updated: 2025-11-12 (nng 1.11 migration)
