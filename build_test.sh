#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Simple build script for the glaze char bug test
# Usage: ./build_test.sh

set -e

echo "Building glaze char bug test..."

# Create build directory
mkdir -p test_build
cd test_build

# Copy CMakeLists.txt for the test
cp ../CMakeLists_test.txt CMakeLists.txt
cp ../glaze_char_bug_test.cpp .

# Configure and build
cmake .
make

echo "Build complete. Running test..."
echo "=================================="

# Run the test
./glaze_char_bug_test

echo "=================================="
echo "Test execution complete."