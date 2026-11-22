#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Run clang-tidy on changed C++ files.
#
# Usage:
#   ./tools/tidy-changed.sh [options] [ref | file...]
#   git clang-tidy [options] [ref | file...]
#
# Options:
#   -fix, --fix          Apply suggested fixes automatically
#   -full, --full        Use exhaustive .clang-tidy-full configuration
#   --staged, --cached   Check staged files in index
#   -h, --help           Show this help message

set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$REPO_ROOT"

if [ ! -d "build" ]; then
    echo "❌ build/ directory not found. Configure with: cmake -S . -G Ninja -B build"
    exit 1
fi

RUN_CLANG_TIDY=""
if [ -f "build/CMakeCache.txt" ]; then
    CACHED_RUN_TIDY=$(sed -n 's/^RUN_CLANG_TIDY_EXE:FILEPATH=//p' build/CMakeCache.txt | head -n 1)
    if [ -n "$CACHED_RUN_TIDY" ] && [ -x "$CACHED_RUN_TIDY" ]; then
        RUN_CLANG_TIDY="$CACHED_RUN_TIDY"
    fi
fi

if [ -z "$RUN_CLANG_TIDY" ]; then
    for candidate in /usr/lib/llvm-22/bin/run-clang-tidy run-clang-tidy-22 run-clang-tidy run-clang-tidy-21 run-clang-tidy-20 run-clang-tidy-19; do
        if command -v "$candidate" >/dev/null 2>&1; then
            RUN_CLANG_TIDY="$(command -v "$candidate")"
            break
        fi
    done
fi

if [ -z "$RUN_CLANG_TIDY" ]; then
    echo "❌ run-clang-tidy not found in PATH"
    exit 1
fi

FIX_FLAG=""
CONFIG_FLAG=""
STAGED_ONLY=false
REF=""
SPECIFIC_FILES=()
CUSTOM_TIDY_BIN="${CLANG_TIDY_BINARY:-}"

while [ $# -gt 0 ]; do
    arg="$1"
    case "$arg" in
        -h|--help)
            echo "Usage: git clang-tidy [options] [ref | file...]"
            echo "       ./tools/tidy-changed.sh [options] [ref | file...]"
            echo ""
            echo "Options:"
            echo "  -fix, --fix                 Apply suggested fixes automatically"
            echo "  -full, --full               Use exhaustive .clang-tidy-full configuration"
            echo "  --staged, --cached          Check staged files in index"
            echo "  --clang-tidy-binary <bin>   Use custom clang-tidy binary (e.g. clang_tidy)"
            echo "  -h, --help                  Show this help message"
            exit 0
            ;;
        -fix|--fix)
            FIX_FLAG="-fix"
            shift
            ;;
        -full|--full)
            CONFIG_FLAG="-config-file=.clang-tidy-full"
            shift
            ;;
        --staged|--cached)
            STAGED_ONLY=true
            shift
            ;;
        --clang-tidy-binary|-clang-tidy-binary)
            CUSTOM_TIDY_BIN="$2"
            shift 2
            ;;
        *)
            if [ -f "$arg" ]; then
                SPECIFIC_FILES+=("$arg")
            else
                REF="$arg"
            fi
            shift
            ;;
    esac
done

FILES=""
if [ ${#SPECIFIC_FILES[@]} -gt 0 ]; then
    FILES="${SPECIFIC_FILES[*]}"
elif [ -n "$REF" ]; then
    FILES=$(git diff --name-only --diff-filter=d "$REF"...HEAD | grep -E '\.(cc|cppm)$' || true)
elif [ "$STAGED_ONLY" = true ]; then
    FILES=$(git diff --cached --name-only --diff-filter=d | grep -E '\.(cc|cppm)$' || true)
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

if [ -f "build/compile_commands.json" ]; then
    mapfile -t NINJA_TARGETS < <(python3 -c '
import json, os, sys
build_dir = os.path.realpath("build")
wanted = {os.path.realpath(p) for p in sys.argv[1:]}
with open("build/compile_commands.json", encoding="utf-8") as f:
    entries = json.load(f)
seen = []
for entry in entries:
    out = entry.get("output")
    if out and os.path.realpath(entry.get("file", "")) in wanted:
        rel = os.path.relpath(out, build_dir) if os.path.isabs(out) else out
        if rel not in seen:
            seen.append(rel)
if seen:
    print("\n".join(seen))
' $FILES)
    if [ ${#NINJA_TARGETS[@]} -gt 0 ]; then
        echo "🔨 Building module prerequisites for changed files..."
        ninja -C build "${NINJA_TARGETS[@]}"
    fi
fi

if [ -z "$CUSTOM_TIDY_BIN" ]; then
    VERSIONED_TIDY="${RUN_CLANG_TIDY/run-clang-tidy/clang-tidy}"
    SIBLING_TIDY="$(dirname "$RUN_CLANG_TIDY")/clang-tidy"
    if [ -x "$VERSIONED_TIDY" ]; then
        CUSTOM_TIDY_BIN="$VERSIONED_TIDY"
    elif [ -x "$SIBLING_TIDY" ]; then
        CUSTOM_TIDY_BIN="$SIBLING_TIDY"
    fi
fi

TIDY_ARGS=("-p" "build" "-header-filter=.*(gb|dallib)/.*" "-exclude-header-filter=.*third_party/.*" "-quiet")
if [ -n "$CUSTOM_TIDY_BIN" ]; then
    TIDY_ARGS+=("-clang-tidy-binary" "$CUSTOM_TIDY_BIN")
fi
if [ -n "$CONFIG_FLAG" ]; then
    TIDY_ARGS+=("$CONFIG_FLAG")
fi
if [ -n "$FIX_FLAG" ]; then
    TIDY_ARGS+=("$FIX_FLAG")
fi

"$RUN_CLANG_TIDY" "${TIDY_ARGS[@]}" $FILES
