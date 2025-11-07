#!/bin/bash
# Generate code for all languages

set -e

echo "Generating PluQ protocol code..."

# Check if flatc is installed
if ! command -v flatc &> /dev/null; then
    echo "Error: flatc (FlatBuffers compiler) not found"
    echo "Install it from: https://github.com/google/flatbuffers"
    exit 1
fi

# Create output directories
mkdir -p ../c/generated
mkdir -p ../csharp/Generated
mkdir -p ../cpp/generated

# Generate C code
echo "Generating C code..."
flatc --c -o ../c/generated pluq.fbs

# Generate C# code
echo "Generating C# code..."
flatc --csharp -o ../csharp/Generated pluq.fbs

# Generate C++ code
echo "Generating C++ code..."
flatc --cpp -o ../cpp/generated pluq.fbs

echo "Code generation complete!"
echo ""
echo "Generated files:"
echo "  C:   c/generated/pluq_generated.h"
echo "  C#:  csharp/Generated/PluQ/*.cs"
echo "  C++: cpp/generated/pluq_generated.h"
