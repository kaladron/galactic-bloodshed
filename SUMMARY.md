# Glaze char/unsigned char Bug Test Case - Summary

## What Was Created

I've created a complete test case to reproduce the glaze char/unsigned char serialization bug that was discovered in PR #13 and worked around in PR #14 of galactic-bloodshed.

## Files Created

1. **`glaze_char_bug_simple.cpp`** (67 lines) - The minimal reproduction case
   - Perfect for copying and pasting into a bug report
   - Shows the issue with both `char` and `unsigned char` fields
   - Clear output showing expected vs actual behavior

2. **`glaze_char_bug_test.cpp`** (140+ lines) - Comprehensive test with detailed diagnostics  
   - More verbose output for debugging
   - Detailed error reporting
   - Clear explanation of the expected behavior

3. **`CMakeLists_test.txt`** - CMake configuration for C++23 build
   - Uses FetchContent to automatically get glaze
   - Compatible with C++23 (not C++26 like the main project)

4. **`build_test.sh`** - Simple build script
   - Uses temporary directories to avoid cluttering the repo
   - Automatically builds and runs the test

5. **`GLAZE_BUG_README.md`** - Complete documentation
   - Multiple build options
   - Usage instructions  
   - Bug description and context

6. **`.gitignore`** updates - Added entries to prevent build artifacts from being committed

## How to Use for Bug Reporting

### Quick Copy-Paste Option
Copy the entire contents of `glaze_char_bug_simple.cpp` into a GitHub issue. It's completely self-contained and demonstrates the problem clearly.

### Test Locally
```bash
# If you have this repo:
./build_test.sh

# Or manually in any directory:
# 1. Copy glaze_char_bug_simple.cpp to your test directory
# 2. Create a simple CMakeLists.txt that fetches glaze
# 3. Build and run
```

## The Bug

The bug occurs when using glaze's reflection-based JSON serialization with `char` and `unsigned char` fields. These fields either:
- Fail to serialize properly
- Produce incorrect values during deserialization  
- Cause serialization/deserialization errors

## Context from galactic-bloodshed

- **PR #13**: Added JSON serialization to Race class, discovered the char/unsigned char bug
- **PR #14**: Worked around the bug by converting char/unsigned char fields to bool (where they were used as boolean flags)

## Expected Outcome

When you run the test cases, they should demonstrate the bug. If they pass, it means either:
1. The bug has been fixed in the current version of glaze
2. The bug manifests differently in your environment  
3. The test needs refinement

## Ready for Upstream Reporting

These test cases are ready to be used to file a bug report with the glaze project maintainers. The simple version is particularly suitable for GitHub issues, while the comprehensive version can be used for more detailed analysis.