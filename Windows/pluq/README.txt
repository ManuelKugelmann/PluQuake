# PluQ Dependencies for Windows

This directory contains pre-built nng and flatcc libraries for Windows.

## Versions
- nng: 2.0.0-alpha.6
- flatcc: master

## Build Information
- Built with: Visual Studio 2022
- Configuration: Release
- Architectures: x86, x64

## Contents
- include/nng/     - nng headers
- include/flatcc/  - flatcc headers
- lib/x86/         - 32-bit libraries (nng.lib, flatccrt.lib)
- lib/x64/         - 64-bit libraries (nng.lib, flatccrt.lib)

## Rebuild
To rebuild these libraries, run:
  .\build-pluq-libs.ps1

## Sources
- nng: https://github.com/nanomsg/nng
- flatcc: https://github.com/dvidelabs/flatcc

Built on: 2025-11-11 22:15:23
