# Build nng and flatcc libraries for Windows (x86 and x64, MSVC and MinGW)
# This follows the pattern used for other Windows dependencies in this repo

param(
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

# Versions
$NNG_VERSION = "1.11"
$FLATCC_VERSION = "master"  # Use master branch - actively maintained with modern CMake support

# Directories
$ScriptDir = $PSScriptRoot
$WorkDir = Join-Path $env:TEMP "pluq_build"
$OutputDir = Join-Path $ScriptDir "pluq"

Write-Host "=========================================="
Write-Host "  Building PluQ Dependencies for Windows"
Write-Host "=========================================="
Write-Host ""
Write-Host "nng version: $NNG_VERSION"
Write-Host "flatcc version: $FLATCC_VERSION"
Write-Host "Output directory: $OutputDir"
Write-Host ""

# Create directories
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null
New-Item -ItemType Directory -Force -Path "$OutputDir/include" | Out-Null
New-Item -ItemType Directory -Force -Path "$OutputDir/lib/x86" | Out-Null
New-Item -ItemType Directory -Force -Path "$OutputDir/lib/x64" | Out-Null

# Download sources
Write-Host "Downloading sources..."

# nng
$nngUrl = "https://github.com/nanomsg/nng/archive/refs/tags/v$NNG_VERSION.zip"
$nngZip = Join-Path $WorkDir "nng.zip"
Invoke-WebRequest -Uri $nngUrl -OutFile $nngZip
Expand-Archive -Path $nngZip -DestinationPath $WorkDir -Force
$nngSource = Join-Path $WorkDir "nng-$NNG_VERSION"

# flatcc - download master branch
$flatccUrl = "https://github.com/dvidelabs/flatcc/archive/refs/heads/$FLATCC_VERSION.zip"
$flatccZip = Join-Path $WorkDir "flatcc.zip"
Invoke-WebRequest -Uri $flatccUrl -OutFile $flatccZip
Expand-Archive -Path $flatccZip -DestinationPath $WorkDir -Force
$flatccSource = Join-Path $WorkDir "flatcc-$FLATCC_VERSION"

# Find CMake and Visual Studio
Write-Host "`nLocating build tools..."
$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Path
if (-not $cmake) {
    throw "CMake not found. Please install CMake and add it to PATH."
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "Visual Studio 2017 or later not found."
}

Write-Host "CMake: $cmake"
Write-Host "Visual Studio: Found"

# Build function
function Build-Library {
    param(
        [string]$Name,
        [string]$SourceDir,
        [string]$Platform,
        [hashtable]$CMakeArgs = @{}
    )

    Write-Host "`n=========================================="
    Write-Host "Building $Name ($Platform)"
    Write-Host "=========================================="

    $arch = if ($Platform -eq "x64") { "x64" } else { "Win32" }
    $buildDir = Join-Path $WorkDir "build-$Name-$Platform"

    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
    Push-Location $buildDir

    try {
        # Configure
        $cmakeArgsArray = @(
            "-G", "Visual Studio 17 2022",
            "-A", $arch,
            "-DCMAKE_BUILD_TYPE=$BuildType",
            "-DCMAKE_INSTALL_PREFIX=$buildDir/install"
        )

        foreach ($key in $CMakeArgs.Keys) {
            $cmakeArgsArray += "-D$key=$($CMakeArgs[$key])"
        }

        $cmakeArgsArray += $SourceDir

        Write-Host "CMake configure..."
        & $cmake $cmakeArgsArray | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

        # Build
        Write-Host "CMake build..."
        & $cmake --build . --config $BuildType | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "CMake build failed" }

        # Install
        Write-Host "CMake install..."
        & $cmake --install . --config $BuildType | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "CMake install failed" }

    } finally {
        Pop-Location
    }

    return Join-Path $buildDir "install"
}

# Build nng
$nngInstallX86 = Build-Library -Name "nng" -SourceDir $nngSource -Platform "x86" -CMakeArgs @{
    "BUILD_SHARED_LIBS" = "OFF"
    "NNG_TESTS" = "OFF"
    "NNG_TOOLS" = "OFF"
    "NNG_ENABLE_TLS" = "OFF"
}

$nngInstallX64 = Build-Library -Name "nng" -SourceDir $nngSource -Platform "x64" -CMakeArgs @{
    "BUILD_SHARED_LIBS" = "OFF"
    "NNG_TESTS" = "OFF"
    "NNG_TOOLS" = "OFF"
    "NNG_ENABLE_TLS" = "OFF"
}

# Build flatcc
$flatccInstallX86 = Build-Library -Name "flatcc" -SourceDir $flatccSource -Platform "x86" -CMakeArgs @{
    "FLATCC_INSTALL" = "ON"
    "FLATCC_RTONLY" = "ON"
}

$flatccInstallX64 = Build-Library -Name "flatcc" -SourceDir $flatccSource -Platform "x64" -CMakeArgs @{
    "FLATCC_INSTALL" = "ON"
    "FLATCC_RTONLY" = "ON"
}

# Copy files to output directory
Write-Host "`n=========================================="
Write-Host "Copying files to $OutputDir"
Write-Host "=========================================="

# Headers
Write-Host "Copying headers..."
Copy-Item -Path "$nngInstallX64/include/nng" -Destination "$OutputDir/include/nng" -Recurse -Force
Copy-Item -Path "$flatccInstallX64/include/flatcc" -Destination "$OutputDir/include/flatcc" -Recurse -Force

# Libraries x86
Write-Host "Copying x86 libraries..."
Copy-Item -Path "$nngInstallX86/lib/nng.lib" -Destination "$OutputDir/lib/x86/" -Force
Copy-Item -Path "$flatccInstallX86/lib/flatccrt.lib" -Destination "$OutputDir/lib/x86/" -Force

# Libraries x64
Write-Host "Copying x64 libraries..."
Copy-Item -Path "$nngInstallX64/lib/nng.lib" -Destination "$OutputDir/lib/x64/" -Force
Copy-Item -Path "$flatccInstallX64/lib/flatccrt.lib" -Destination "$OutputDir/lib/x64/" -Force

# Create README
$readme = @"
# PluQ Dependencies for Windows

This directory contains pre-built nng and flatcc libraries for Windows.

## Versions
- nng: $NNG_VERSION
- flatcc: $FLATCC_VERSION

## Build Information
- Built with: Visual Studio 2022
- Configuration: $BuildType
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

Built on: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
"@

Set-Content -Path "$OutputDir/README.txt" -Value $readme

Write-Host "`n=========================================="
Write-Host "  Build Complete!"
Write-Host "=========================================="
Write-Host ""
Write-Host "Output directory: $OutputDir"
Write-Host ""
Write-Host "Libraries built:"
Write-Host "  x86: nng.lib, flatccrt.lib"
Write-Host "  x64: nng.lib, flatccrt.lib"
Write-Host ""
Write-Host "Next steps:"
Write-Host "1. Update ironwail.vcxproj to include these paths"
Write-Host "2. Commit the Windows/pluq directory to the repository"
Write-Host ""
