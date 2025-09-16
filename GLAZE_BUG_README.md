# Glaze char/unsigned char Serialization Bug Test Case

This directory contains a minimal test case to reproduce a bug in the [glaze](https://github.com/stephenberry/glaze) JSON serialization library where `char` and `unsigned char` fields are not handled correctly during serialization/deserialization.

## Bug Description

When using glaze's reflection-based JSON serialization, fields of type `char` and `unsigned char` do not serialize/deserialize correctly. This was discovered while implementing JSON serialization for the Race class in galactic-bloodshed.

## Files

- `glaze_char_bug_test.cpp` - The main test program (standalone, single file)
- `CMakeLists_test.txt` - CMake configuration for building the test
- `build_test.sh` - Simple build script 
- `GLAZE_BUG_README.md` - This documentation

## How to Build and Run

### Option 1: Using the build script (recommended)
```bash
./build_test.sh
```
This will create a temporary build directory, build the test, run it, and show you the temporary directory location.

### Option 2: Manual build in temporary directory
```bash
# Create temporary directory for source files
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

# Copy files
cp /path/to/CMakeLists_test.txt CMakeLists.txt
cp /path/to/glaze_char_bug_test.cpp .

# Create build directory and configure
mkdir build && cd build
cmake ..
make

# Run
./glaze_char_bug_test
```

### Option 3: Standalone compilation (if glaze is already installed)
```bash
g++ -std=c++23 -I/path/to/glaze/include glaze_char_bug_test.cpp -o glaze_char_bug_test
./glaze_char_bug_test
```

## Expected vs Actual Behavior

**Expected**: The test should serialize a struct containing `char` and `unsigned char` fields to JSON, then deserialize it back, with all values preserved.

**Actual**: The serialization/deserialization fails or produces incorrect values for the `char` and `unsigned char` fields.

## Workaround

The workaround used in galactic-bloodshed was to change the `char`/`unsigned char` fields to `bool` where appropriate, since the fields were being used as boolean flags anyway.

## Environment

- C++23 compatible compiler
- glaze library (fetched automatically via CMake)
- Tested with: [specify compiler and glaze version when reporting]

## Reporting the Bug

Copy the contents of `glaze_char_bug_test.cpp` when filing an issue with the glaze project. The test is self-contained and should reproduce the problem reliably.