---
name: clang-format
description: 'Format modified C++ source files using clang-format before committing or pushing code. Use whenever preparing to commit, push, or format C++ code in the repository.'
user-invocable: true
---

# Clang Format

Use this skill when preparing to commit, push, or format C++ code in this repository.

## Required Behavior

1. **Format modified files or diffs**:
   - Run `git clang-format` on staged changes or `git clang-format -f` on unstaged changes before committing or pushing.
   - Alternatively, run `clang-format -i <file>` on each modified C++ file (`.cc`, `.cppm`, `.h`, `.hpp`).
2. Verify formatting using `git diff`.
3. Ensure the project builds (`cmake --build build`) and tests pass (`(cd build && ctest)`) after formatting.

