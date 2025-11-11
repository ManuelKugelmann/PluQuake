# Building PluQ Dependencies for Windows

PluQ requires nng and flatcc libraries. These need to be built for Windows.

## Quick Start

Run the build script in PowerShell:

```powershell
cd Windows
.\build-pluq-libs.ps1
```

This will:
1. Download nng v2.0.0-alpha.6 and flatcc v0.6.1
2. Build both for x86 and x64 architectures
3. Create `Windows/pluq/` with headers and libraries
4. Generate a README with build information

## Requirements

- Visual Studio 2017 or later
- CMake 3.7 or later
- Internet connection (to download sources)

## Output Structure

```
Windows/pluq/
├── include/
│   ├── nng/        # nng headers
│   └── flatcc/     # flatcc headers
├── lib/
│   ├── x86/        # 32-bit libraries
│   │   ├── nng.lib
│   │   └── flatccrt.lib
│   └── x64/        # 64-bit libraries
│       ├── nng.lib
│       └── flatccrt.lib
└── README.txt
```

## Integration

The Visual Studio project (`VisualStudio/ironwail.vcxproj`) is already configured to:
- Include headers from `../pluq/include`
- Link libraries from `../pluq/lib/$(PlatformShortName)`
- Add `-DUSE_PLUQ` define

## Manual Build (Alternative)

If you prefer to build manually or customize the build:

### nng
```powershell
git clone -b v2.0.0-alpha.6 https://github.com/nanomsg/nng.git
cd nng
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 -DBUILD_SHARED_LIBS=OFF -DNNG_TESTS=OFF -DNNG_TOOLS=OFF ..
cmake --build . --config Release
cmake --install . --prefix ../install
```

### flatcc
```powershell
git clone -b v0.6.1 https://github.com/dvidelabs/flatcc.git
cd flatcc
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 -DFLATCC_INSTALL=ON -DFLATCC_RTONLY=ON ..
cmake --build . --config Release
cmake --install . --prefix ../install
```

Then copy headers and .lib files to the `Windows/pluq/` structure shown above.

## Troubleshooting

**CMake not found**: Install CMake from https://cmake.org/download/ or via Visual Studio Installer

**Build fails**: Make sure you have the "Desktop development with C++" workload installed in Visual Studio

**Wrong Visual Studio version**: Edit the script and change the generator from "Visual Studio 17 2022" to your version

## Notes

- Libraries are built as **static** (.lib) to match the existing pattern in this repository
- TLS support in nng is disabled to reduce dependencies
- Only the flatcc runtime is built (FLATCC_RTONLY), not the compiler tools
