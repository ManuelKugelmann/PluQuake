#!/bin/bash
# Download Quake shareware pak0.pak for testing
# Source: https://github.com/pweil-/origin-quake (direct pak0.pak)
# This is the freely distributable shareware version (Episode 1)

set -e
cd "$(dirname "$0")"

if [ -f "id1/pak0.pak" ]; then
    echo "Quake shareware already downloaded"
    exit 0
fi

echo "Downloading Quake shareware pak0.pak..."

# Create id1 directory
mkdir -p id1

# Download pak0.pak directly from origin-quake repository
PAK_URL="https://github.com/pweil-/origin-quake/raw/master/id1/pak0.pak"

# Try curl first (follows redirects with -L), then wget
if command -v curl &> /dev/null; then
    curl -sL -o id1/pak0.pak "$PAK_URL"
elif command -v wget &> /dev/null; then
    wget -q -O id1/pak0.pak "$PAK_URL"
else
    echo "ERROR: Neither curl nor wget found"
    exit 1
fi

# Verify download (should be ~18MB)
if [ ! -f "id1/pak0.pak" ]; then
    echo "ERROR: Download failed"
    exit 1
fi

FILESIZE=$(stat -c%s "id1/pak0.pak" 2>/dev/null || stat -f%z "id1/pak0.pak" 2>/dev/null || echo "0")
if [ "$FILESIZE" -lt 1000000 ]; then
    echo "ERROR: Downloaded file too small ($FILESIZE bytes), download may have failed"
    rm -f id1/pak0.pak
    exit 1
fi

echo "Quake shareware downloaded successfully"
ls -la id1/pak0.pak
