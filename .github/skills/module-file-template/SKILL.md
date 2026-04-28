---
name: module-file-template
description: 'Apply the C++26 module file structure used throughout this codebase. Use when creating any new .cc, .cppm, or test file, deciding between import gblib/import std vs include, choosing module partition names, or fixing module-related build errors. Covers global module fragment, import order, partition syntax, and when (rarely) #include is allowed.'
user-invocable: false
---

# Module File Template

This codebase uses C++ Modules end-to-end. Every new translation unit follows a strict shape so the build stays clean and partition discovery works.

## Implementation File (.cc)

```cpp
// SPDX-License-Identifier: Apache-2.0

module;                       // Global module fragment begins

#include "gb/files.h"         // (optional) legacy headers ONLY here

import gblib;                 // Project module
import std;                   // Standard library

module commands;              // Module/partition this file belongs to

namespace GB::commands {
// implementation
}  // namespace GB::commands
```

Rules:

1. SPDX header on line 1.
2. `module;` opens the global module fragment. **Only place** `#include` may appear is between `module;` and any `import`.
3. `import gblib;` first, then `import std;` (prefer over `import std.compat;` in new code).
4. `module <name>;` declares which named module/partition this file implements. Must come **after** all imports.
5. Tests typically also need `import dallib;`.

## Module Interface Partition (.cppm)

```cpp
// SPDX-License-Identifier: Apache-2.0

export module gblib:partition_name;   // Or: export module dallib;

import std;
import :types;                         // Other partitions of same module
import :services;

export class Foo { /* ... */ };
export void bar();
```

Rules:

- Single `export module ...` declaration per file.
- Within `gblib`, partitions are referenced as `:partition` (e.g. `:types`, `:ships`).
- Mark every public symbol with `export`. Internal helpers stay unmarked.
- No `#include` in interface partitions except inside `module;` if a legacy header is unavoidable.

## Test File

```cpp
// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  // ... assertions ...
  std::println("Test passed!");
  return 0;
}
```

`<cassert>` may be included after the imports (assert is a macro, so import std doesn't expose it). See the database-test-pattern skill for the full test pattern.

## When `#include` Is Allowed

Only for legacy artifacts that have not yet been modularized:

- `gb/files.h` (path constants, file-name macros)
- `gb/buffers.h` (legacy buffer macros)
- `<cassert>` in tests

Anything else should be reached via `import gblib`, `import dallib`, `import std`, or `import commands`.

## Adding a Source File to the Build

In `gb/CMakeLists.txt`, append the path under the appropriate target:

```cmake
target_sources(commands PRIVATE
  commands/foo.cc
  commands/bar.cc
)
```

Module interface partitions are added with `FILE_SET CXX_MODULES`:

```cmake
target_sources(gblib PUBLIC
  FILE_SET CXX_MODULES FILES
    gblib-foo.cppm
)
```

Match the pattern of nearby entries — don't invent a new shape.

## Common Build Errors and Fixes

| Symptom | Cause | Fix |
| --- | --- | --- |
| "module ... not found" | Missing partition import or partition not in CMake | Add `import :name;` and add `.cppm` to `FILE_SET CXX_MODULES` |
| "cannot use 'export' in non-module" | Missing `export module ...;` | Add the module declaration before `export` |
| "include after module" | `#include` placed after an `import` | Move includes inside the `module;` fragment |
| Symbol not found at link time | Symbol not marked `export` | Add `export` in the .cppm |

## Anti-Patterns

- ❌ Mixing `#include` for project headers when an `import` exists.
- ❌ Both `import std;` and `import std.compat;` in the same file.
- ❌ Declaring a function in a `.cppm` without `export` then using it from another TU.
- ❌ Putting includes after imports.
- ❌ Multiple `module xxx;` lines per file.

## Checklist

- [ ] SPDX header on line 1
- [ ] Includes (if any) are inside `module;` fragment
- [ ] `import gblib;` before `import std;`
- [ ] `import std;` (not `import std.compat;`) for new code
- [ ] Single `module <name>;` after imports
- [ ] All public `.cppm` symbols are `export`-ed
- [ ] File registered in `gb/CMakeLists.txt`
