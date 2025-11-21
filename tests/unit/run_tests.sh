#!/bin/bash

# Build and run tests for climate_ir_woleix component

set -e

echo "=== Building Woleix Climate IR Unit Tests ==="

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
./climate_ir_woleix_test

echo ""
echo "=== Tests Complete ==="
