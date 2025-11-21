#!/bin/bash

# Generate code coverage report for climate_ir_woleix component

set -e

echo "=== Generating Code Coverage Report ==="

# Ensure we're in the build directory
if [ ! -d "build" ]; then
    echo "Error: build directory not found. Please run run_tests.sh first."
    exit 1
fi

cd build

# Check if coverage data exists
if [ ! -f "CMakeFiles/climate_ir_woleix_test.dir/climate_ir_woleix_test.cpp.gcda" ]; then
    echo "Error: No coverage data found. Make sure tests were built with -DENABLE_COVERAGE=ON and executed."
    exit 1
fi

echo "Capturing coverage data..."

# Create coverage directory
mkdir -p coverage

# Capture coverage data with error handling
lcov --capture \
    --directory . \
    --output-file coverage/coverage.info \
    --rc branch_coverage=1 \
    --ignore-errors inconsistent,inconsistent \
    --ignore-errors mismatch,mismatch \
    --ignore-errors format,format \
    --ignore-errors unsupported,unsupported

# Filter out system headers, test files, and external dependencies
# Keep only the component source files
lcov --extract coverage/coverage.info \
    '*/esphome/components/climate_ir_woleix/climate_ir_woleix.cpp' \
    '*/esphome/components/climate_ir_woleix/climate_ir_woleix.h' \
    --output-file coverage/coverage_filtered.info \
    --rc branch_coverage=1 \
    --ignore-errors inconsistent,inconsistent \
    --ignore-errors unused,unused \
    --ignore-errors format,format

# Generate HTML report
genhtml coverage/coverage_filtered.info \
    --output-directory coverage/html \
    --title "Climate IR Woleix Coverage Report" \
    --num-spaces 4 \
    --legend \
    --rc branch_coverage=1 \
    --ignore-errors source,source \
    --ignore-errors category,category

echo ""
echo "=== Coverage Report Generated ==="
echo "HTML report: build/coverage/html/index.html"
echo ""

# Display summary
lcov --summary coverage/coverage_filtered.info --rc branch_coverage=1
