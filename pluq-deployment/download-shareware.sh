#!/bin/bash
# Download Quake shareware for testing PluQ system

set -e

cd "$(dirname "$0")"

echo "=== Downloading Quake Shareware ==="
echo ""

# Create id1 directory if it doesn't exist
mkdir -p id1

# Check if pak0.pak already exists
if [ -f "id1/pak0.pak" ]; then
    echo "✅ pak0.pak already exists"
    ls -lh id1/pak0.pak
    exit 0
fi

echo "Downloading Quake pak files from archive.org (~23 MB)..."
echo "(Note: dosgamesarchive.com would be preferred but doesn't work through proxy)"
wget -q --show-progress -O /tmp/quake-paks.zip \
    "https://archive.org/download/quake_pak_202306/quake_pak.zip"

echo ""
echo "Extracting pak0.pak..."
unzip -j /tmp/quake-paks.zip "pak0.pak" -d id1/

echo ""
echo "Cleaning up..."
rm /tmp/quake-paks.zip

echo ""
echo "✅ Quake shareware installed successfully!"
ls -lh id1/pak0.pak
echo ""
echo "You can now run the test scripts:"
echo "  ./test-pluq.sh"
echo "  ./test-backend.sh"
