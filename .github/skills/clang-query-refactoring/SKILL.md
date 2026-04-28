---
name: clang-query-refactoring
description: 'Use clang-query AST matchers to find every call site that needs to change before a codebase-wide refactor (e.g. migrating direct field access to accessor methods, finding all uses of a deprecated API). Use when a refactor touches dozens of files and grep would over-match, when verifying every mutation site has been migrated, or when planning a safe API change. Covers compile_commands.json setup, writing matchers, running across the tree, and pairing matcher output with replacement.'
user-invocable: false
---

# clang-query Refactoring

`clang-query` matches against the real Clang AST, so it understands C++ types, overloads, and templates. That makes it dramatically safer than grep for cross-cutting refactors — the kind that show up in this codebase when migrating raw struct fields to accessor methods, or replacing a deprecated function across dozens of TUs.

## Prerequisites

- `clang-query-22` (matches the project’s clang). Other versions usually work but the matcher syntax can drift.
- A populated `compile_commands.json` at the workspace root (CMake generates it automatically — re-run `cmake -S . -B build` if missing).
- The code currently **compiles**. clang-query needs valid AST. If you have already removed the symbol you want to find, revert that commit temporarily.

## Workflow

1. Confirm the build is green.
2. Write a matcher in a query file.
3. Run it across the affected files.
4. Apply edits at each match (manually or scripted).
5. Build & test; iterate.

## Matcher Anatomy

Match expressions accessing specific fields on a specific class:

```text
# /tmp/sector_query.txt
set output diag

match memberExpr(
  hasObjectExpression(
    hasType(hasUnqualifiedDesugaredType(
      recordType(hasDeclaration(cxxRecordDecl(hasName("Sector"))))))),
  member(fieldDecl(anyOf(
    hasName("x"),
    hasName("y"),
    hasName("eff"),
    hasName("popn")))))
```

Reading the matcher top-down:

- `memberExpr` — any `obj.field` or `obj->field`.
- `hasObjectExpression(hasType(...))` — constrains the object’s type.
- `hasUnqualifiedDesugaredType(recordType(...))` — strips `const`, `&`, typedefs to reach the underlying class.
- `cxxRecordDecl(hasName("Sector"))` — pin to a specific class.
- `member(fieldDecl(anyOf(...)))` — pin to specific fields, excluding methods.

`set output diag` makes results print as compiler-style diagnostics with file/line/column, easy to scan or post-process.

## Running

```bash
clang-query-22 -p build -f /tmp/sector_query.txt gb/some_file.cppm
```

`-p build` points at the build directory containing `compile_commands.json`. Pass one or more source files as arguments.

To sweep the codebase:

```bash
find gb -name '*.cc' -o -name '*.cppm' | while read f; do
  clang-query-22 -p build -f /tmp/sector_query.txt "$f" 2>/dev/null
done | tee /tmp/matches.txt
```

Pipe through `grep "note:"` to extract just the locations.

## Common Matchers

### Calls to a specific function

```text
match callExpr(callee(functionDecl(hasName("getship"))))
```

### Construction of a deprecated type

```text
match cxxConstructExpr(hasType(cxxRecordDecl(hasName("OldRequest"))))
```

### Implicit conversions between strong IDs

```text
match implicitCastExpr(
  hasSourceExpression(hasType(asString("int"))),
  hasType(cxxRecordDecl(hasName("ID"))))
```

### Members written-to (left-hand side of assignment)

```text
match binaryOperator(
  hasOperatorName("="),
  hasLHS(memberExpr(member(fieldDecl(hasName("popn"))))))
```

## Pairing With Edits

`clang-query` finds, but does not rewrite. After collecting matches:

- Manual: open each file at the reported line/column, apply the targeted change, re-run the query — count goes to zero.
- Scripted: feed the locations into `sed`/`comby`/`clang-tidy --fix`. For mechanical text replacements `comby` is usually the cleanest follow-up.

After every batch:

```bash
ninja -C build && (cd build && ctest)
```

If the matcher finds a real call site that the refactor missed, the build will fail loudly — that is the point.

## Anti-Patterns

- ❌ Running clang-query against code that does not currently compile (no AST → no matches).
- ❌ Using grep alone for class-aware refactors — overlapping field names across unrelated types will mislead you.
- ❌ Forgetting `hasUnqualifiedDesugaredType(...)` and missing `const Sector&` / `Sector*` accesses.
- ❌ Trusting a "0 matches" result without first verifying the matcher fires on a known case.
- ❌ Committing the temporary revert commit you used to make the code compile.

## Checklist

- [ ] Build is green; `compile_commands.json` is current
- [ ] Matcher prints at least one expected hit on a known site (sanity check)
- [ ] Sweep covers every relevant `.cc` / `.cppm`
- [ ] Edits applied; matcher re-run shows zero remaining hits
- [ ] `ninja -C build` and `ctest` both pass
- [ ] Any temporary revert used to enable AST analysis has been undone
