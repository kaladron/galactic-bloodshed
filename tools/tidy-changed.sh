#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Run clang-tidy on changed C++ files.
#
# Usage:
#   ./tools/tidy-changed.sh                # Checks unstaged/staged changed files vs HEAD
#   ./tools/tidy-changed.sh origin/main    # Checks changed files against origin/main
#   ./tools/tidy-changed.sh file1.cc ...   # Checks specific files

set -euo pipefail

RUN_CLANG_TIDY=""
for candidate in run-clang-tidy run-clang-tidy-22 run-clang-tidy-21 run-clang-tidy-20 run-clang-tidy-19; do
    if command -v "$candidate" >/dev/null 2>&1; then
        RUN_CLANG_TIDY="$candidate"
        break
    fi
done

if [ -z "$RUN_CLANG_TIDY" ]; then
    echo "❌ run-clang-tidy not found in PATH"
    exit 1
fi

if [ ! -d "build" ]; then
    echo "❌ build/ directory not found. Configure with: cmake -S . -G Ninja -B build"
    exit 1
fi

FILES=""
if [ $# -gt 0 ] && [ -f "$1" ]; then
    FILES="$*"
elif [ $# -gt 0 ]; then
    FILES=$(git diff --name-only --diff-filter=d "$1"...HEAD | grep -E '\.(cc|cppm)$' || true)
else
    FILES=$(git diff --name-only --diff-filter=d HEAD | grep -E '\.(cc|cppm)$' || true)
    if [ -z "$FILES" ]; then
        FILES=$(git diff --cached --name-only --diff-filter=d | grep -E '\.(cc|cppm)$' || true)
    fi
fi

if [ -z "$FILES" ]; then
    echo "✅ No changed C++ files to check with clang-tidy"
    exit 0
fi

echo "🔍 Running clang-tidy on changed files:"
for f in $FILES; do
    echo "   $f"
done

"$RUN_CLANG_TIDY" -p build -header-filter='.*(gb|dallib)/.*' -exclude-header-filter='.*third_party/.*' -quiet $FILES
