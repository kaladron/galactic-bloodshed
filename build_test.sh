#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Simple build script for the glaze char bug test
# Usage: ./build_test.sh

set -e

echo "Building glaze char bug test..."

# Create temporary directory for source files
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

echo "Building in temporary directory: $TEMP_DIR"

# Copy test files to temp directory
cp "$OLDPWD/CMakeLists_test.txt" CMakeLists.txt
cp "$OLDPWD/glaze_char_bug_test.cpp" .

# Create build subdirectory and configure from there
mkdir build
cd build

# Configure and build using out-of-source build
cmake ..
make

echo "Build complete. Running test..."
echo "=================================="

# Run the test
./glaze_char_bug_test

echo "=================================="
echo "Test execution complete."
echo "Temporary build directory: $TEMP_DIR"