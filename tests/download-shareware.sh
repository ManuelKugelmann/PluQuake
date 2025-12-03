#!/bin/bash
# Download Quake shareware pak0.pak for testing
# This is the freely distributable shareware version

set -e
cd "$(dirname "$0")"

if [ -f "id1/pak0.pak" ]; then
    echo "Quake shareware already downloaded"
    exit 0
fi

echo "Downloading Quake shareware..."

# Create id1 directory
mkdir -p id1

# Download from archive.org (official shareware distribution)
SHAREWARE_URL="https://archive.org/download/quake-shareware-1.06/quake106.zip"

# Try wget first, then curl
if command -v wget &> /dev/null; then
    wget -q -O quake106.zip "$SHAREWARE_URL"
elif command -v curl &> /dev/null; then
    curl -sL -o quake106.zip "$SHAREWARE_URL"
else
    echo "ERROR: Neither wget nor curl found"
    exit 1
fi

# Extract pak0.pak from the zip
# The shareware zip contains ID1/PAK0.PAK (uppercase)
if command -v unzip &> /dev/null; then
    unzip -q -o quake106.zip "ID1/PAK0.PAK" -d .
    mv ID1/PAK0.PAK id1/pak0.pak
    rmdir ID1
else
    echo "ERROR: unzip not found"
    exit 1
fi

# Cleanup
rm -f quake106.zip

echo "Quake shareware downloaded successfully"
ls -la id1/pak0.pak
