#!/bin/bash

# Build and run tests for climate_ir_woleix component

set -e

echo "=== Building Woleix Climate Unit Tests ==="

# Create build directory
mkdir -p build
cd build

# Configure with CMake and enable coverage
cmake -DENABLE_COVERAGE=ON ..

# Build
cmake --build .

echo ""
echo "=== Running Tests ==="
echo ""

# Run tests

for f in *_test; do
    [ -f "$f" ] && [ -x "$f" ] && ./"$f"
done

echo ""
echo "=== Tests Complete ==="
