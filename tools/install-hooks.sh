#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Install git hooks for the Galactic Bloodshed project.
# Run this script from the repository root after cloning:
#   ./tools/install-hooks.sh

set -e

REPO_ROOT=$(git rev-parse --show-toplevel)
HOOKS_DIR="$REPO_ROOT/.git/hooks"
TOOLS_DIR="$REPO_ROOT/tools"

echo "📦 Installing git hooks..."

# Install pre-commit hook
if [ -f "$TOOLS_DIR/pre-commit" ]; then
    ln -sf ../../tools/pre-commit "$HOOKS_DIR/pre-commit"
    chmod +x "$TOOLS_DIR/pre-commit"
    echo "✅ Installed pre-commit hook (format checker)"
else
    echo "⚠️  Warning: $TOOLS_DIR/pre-commit not found"
fi

# Configure git aliases
git config alias.clang-tidy '!./tools/tidy-changed.sh'
echo "✅ Configured 'git clang-tidy' alias"

echo ""
echo "✨ Developer environment configured successfully!"
echo ""
echo "The pre-commit hook will check code formatting before each commit."
echo "To bypass a check (not recommended), use: git commit --no-verify"
