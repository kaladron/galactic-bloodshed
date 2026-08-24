---
name: clang-tidy
description: 'Run static analysis checks on modified C++ files using clang-tidy before committing or pushing code. Use whenever preparing to commit, push, or verify static analysis diagnostics.'
user-invocable: true
---

# Clang-Tidy Static Analysis

Use this skill when preparing to commit, push, or verify C++ code against the repository's static analysis rules.

## Required Behavior

1. **Verify changed files before commit**:
   Run the changed-file static analysis command:
   ```bash
   git clang-tidy
   # Or directly:
   ./tools/tidy-changed.sh
   # Or via CMake:
   ninja -C build tidy-changed
   ```
2. **Exhaustive checks (Optional)**:
   To run the full suite across changed files or specific files:
   ```bash
   git clang-tidy --full
   # Or for a specific file:
   run-clang-tidy -p build -config-file=.clang-tidy-full path/to/file.cc
   ```
3. **Auto-fix diagnostics**:
   To automatically apply suggestions for modified files:
   ```bash
   git clang-tidy -fix
   # Or for a specific file:
   run-clang-tidy -p build -fix path/to/file.cc
   ```
4. **Verification**:
   Ensure 0 warnings are emitted, clean builds (`cmake --build build`), and all tests pass (`(cd build && ctest)`).
