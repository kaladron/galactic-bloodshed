#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Build script for glaze char bug tests
# Usage: ./build_test.sh  
# Tests both comprehensive and simple versions

set -e

echo "Building and testing glaze char bug test cases..."
echo "=================================================="

# Create temporary directory for source files
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

echo "Building in temporary directory: $TEMP_DIR"

# Copy test files to temp directory
cp "$OLDPWD/CMakeLists_test.txt" .
cp "$OLDPWD/glaze_char_bug_test.cpp" .
cp "$OLDPWD/glaze_char_bug_simple.cpp" .

# Test 1: Comprehensive test
echo ""
echo "=== BUILDING AND TESTING COMPREHENSIVE VERSION ==="
mkdir comprehensive
cd comprehensive

# Copy source file and create CMakeLists.txt for comprehensive test
cp ../glaze_char_bug_test.cpp .
cat > CMakeLists.txt << 'EOF'
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.25)
project(glaze_char_bug_test LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
include(FetchContent)
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
)
FetchContent_MakeAvailable(glaze)
add_executable(glaze_char_bug_test glaze_char_bug_test.cpp)
target_link_libraries(glaze_char_bug_test PRIVATE glaze::glaze)
EOF

# Build comprehensive test with separate build directory
mkdir build && cd build
cmake ..
make

echo ""
echo "--- Running comprehensive test ---"
./glaze_char_bug_test
COMPREHENSIVE_RESULT=$?

cd ../..

# Test 2: Simple test
echo ""
echo "=== BUILDING AND TESTING SIMPLE VERSION ==="
mkdir simple
cd simple

# Copy source file and create CMakeLists.txt for simple test
cp ../glaze_char_bug_simple.cpp .
cat > CMakeLists.txt << 'EOF'
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.25)
project(glaze_char_bug_simple LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
include(FetchContent)
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
)
FetchContent_MakeAvailable(glaze)
add_executable(glaze_char_bug_simple glaze_char_bug_simple.cpp)
target_link_libraries(glaze_char_bug_simple PRIVATE glaze::glaze)
EOF

# Build simple test with separate build directory
mkdir build && cd build
cmake ..
make

echo ""
echo "--- Running simple test ---"
./glaze_char_bug_simple
SIMPLE_RESULT=$?

echo ""
echo "=================================================="
echo "BUILD AND TEST SUMMARY:"
echo "Comprehensive test: $([ $COMPREHENSIVE_RESULT -eq 0 ] && echo "PASSED" || echo "FAILED (bug reproduced)")"
echo "Simple test: $([ $SIMPLE_RESULT -eq 0 ] && echo "PASSED" || echo "FAILED (bug reproduced)")"
echo ""
echo "The simple test case is ready to copy-paste for upstream bug reports."
echo "Temporary build directory: $TEMP_DIR"

# Return success if either test demonstrates the bug (failure expected)
exit 0