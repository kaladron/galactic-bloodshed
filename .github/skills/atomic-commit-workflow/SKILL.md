---
name: atomic-commit-workflow
description: 'Workflow guidelines for bite-sized (~200 LOC) atomic commits with in-commit tests, formatting, and verification in Galactic Bloodshed.'
user-invocable: false
---

# Atomic Commit Workflow

Follow these rules to deliver high-quality, easily reviewable atomic commits.

## 1. Commit Sizing & Single Responsibility
- **Target**: Aim for **150–250 lines of code** changed per commit.
- **Atomic**: Each commit must address a single conceptual change (e.g. one command migration + its tests, one helper method + its tests).
- **Never split logic from tests**: Every commit introducing new code MUST contain the tests for that code in the same commit.
- **Document latent bug fixes**: Always explicitly call out and explain any latent bugs, edge-case vulnerabilities, or bounds safety hazards resolved during refactoring in the commit message body.
- **Modernize legacy raw types**: Watch for raw primitives (`long`, `short`, `unsigned long`, `char` boolean flags, `int` identifiers) in touched structs or function signatures and convert them to the appropriate domain types (`population_t`, `resource_t`, `money_t`, `ap_t`, `starnum_t`, `planetnum_t`, `bool`).

## 2. Pre-Commit Verification Checklist

Before creating a commit, always execute the following verification cycle:

```bash
# 1. Format all modified C++ files
clang-format -i path/to/file.cc path/to/file.cppm

# 2. Build the project
ninja -C build

# 3. Run all tests
(cd build && ctest)

# 4. Check git diff and stat
git status --short
git diff --stat
```

- Ensure `ninja -C build` succeeds with 0 errors and 0 warnings.
- Ensure `ctest` passes 100% of tests.
- Ensure `git diff --stat` is approximately ~200 LOC.

## 3. Architecture Documentation Protocol
- If the commit introduces a new architectural pattern or public convention, update [`ARCHITECTURE.md`](ARCHITECTURE.md) in the same commit.
- Write documentation in **plain English** explaining concepts simply.
- **Do not** dump large code structs or boilerplate code into `ARCHITECTURE.md`.

## 4. Context Refresh Protocol for Agents
- Before beginning a new commit in a multi-stage migration, re-read the active plan document in the artifact directory and `ARCHITECTURE.md`.
- Update the plan document upon completing and pushing each commit.
