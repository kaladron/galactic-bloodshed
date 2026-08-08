---
name: clang-format
description: 'Format modified C++ source files using clang-format before committing or pushing code. Use whenever preparing to commit, push, or format C++ code in the repository.'
user-invocable: true
---

# Clang Format

Use this skill when preparing to commit, push, or format C++ code in this repository.

## Required Behavior

1. Identify all modified C++ source files (`.cc`, `.cppm`, `.h`, `.hpp`).
2. Run `clang-format -i <file>` on each modified C++ file before committing or pushing.
3. Verify formatting using `git diff`.
4. Ensure the project builds (`cmake --build build`) and tests pass (`(cd build && ctest)`) after formatting.
