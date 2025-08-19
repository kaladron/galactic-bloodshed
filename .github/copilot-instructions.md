# Copilot Instructions for Galactic Bloodshed

These instructions guide Copilot's code suggestions and Chat answers for this repository.

Project overview (context Copilot should assume)
- Language/toolchain: C++26 with C++ Modules, libc++ (LLVM), CMake. `import std` is enabled; code generally uses `import std.compat`.
- Layout:
  - Core engine modules under `gb/` as `gblib:*` module partitions.
  - Player "commands" implemented as free functions in `namespace GB::commands`, exported via `gb/commands/commands.cppm`.
  - Command dispatch table is built in `gb/GB_server.cc::getCommands()`.
- Dependencies: SQLite3 (system package), glaze, scnlib (fetched via CMake FetchContent).
- License headers: All source files start with `// SPDX-License-Identifier: Apache-2.0`.

Coding style and conventions Copilot should follow
- Use C++ Modules, not headers:
  - Start `.cc` files with the canonical module prologue pattern:
    - `module;`
    - `import gblib;`
    - `import std.compat;` (or `import std;` only if strictly needed)
    - (Optionally) `#include "gb/..."` for legacy constants/macros (e.g., `gb/files.h`, `gb/buffers.h`).
    - `module commands;` for command implementations or the relevant `gblib:*` partition for core modules.
- Namespaces and symbols:
  - Commands: implement free functions in `namespace GB::commands { ... }` with signature `void name(const command_t&, GameObj&)`.
  - Do not introduce new global state. Prefer functions and local state.
- I/O and formatting:
  - User-visible output goes to `g.out`. Use `std::format` to build strings and stream them, e.g. `g.out << std::format("...", args...);`.
  - Avoid `printf`, `std::cout`, and ad-hoc formatting. Prefer existing formatters (see custom `std::formatter` specializations in `gblib-types.cppm`).
- Error handling:
  - Prefer early returns with messages written to `g.out`.
  - Use `std::optional` and existing result-returning helpers; check `.has_value()` or result truthiness and print `.error()` where available.
  - Do not throw exceptions in command paths.
- Data access and game state:
  - Read/write game entities via `gblib:files_shl` exports (`getplanet`, `getstar`, `getship`, `putship`, etc.).
  - Use `GameObj`'s scope (`g.level`, `g.snum`, `g.pnum`, `g.player`, `g.governor`) rather than re-deriving state.
  - Use file path constants from `gb/files.h` (e.g., `EXAM_FL`) instead of hard-coded strings.
- Build system and dependencies:
  - Add new command sources to `gb/CMakeLists.txt` in the `commands` target source list.
  - Export command declarations in `gb/commands/commands.cppm`.
  - Wire command names to functions in `gb/GB_server.cc::getCommands()`.
  - Do not introduce new dependencies beyond stdlib, SQLite3, glaze, scnlib.
- Code style tidbits:
  - Keep SPDX header at top of new files.
  - Use consistent indentation and brace style as in existing files.
  - Prefer `std::string_view` for non-owning string params; use `std::optional<T>` for maybe-values.
  - Follow existing `TODO(author):` convention for future work notes.

Patterns Copilot should replicate
- Command skeleton:
  ```c++
  // SPDX-License-Identifier: Apache-2.0
  module;
  import gblib;
  import std.compat;
  module commands;

  namespace GB::commands {
  void example(const command_t& argv, GameObj& g) {
    // Validate scope
    if (g.level != ScopeLevel::LEVEL_SHIP && g.level != ScopeLevel::LEVEL_PLAN) {
      g.out << "You must change scope to a ship or planet.\n";
      return;
    }

    // Resolve inputs, use std::optional-based helpers and early returns
    // Query data via gblib:files_shl (getplanet/getship/etc.)
    // Write user-facing output via g.out << std::format(...);

    // Persist via putship/finish_* helpers as appropriate
  }
  }  // namespace GB::commands
  ```
- Output tables:
  - Use `std::format` with aligned columns (see `commands/build.cc` headers and rows).
- Capability/permission checks:
  - Follow existing checks (e.g., technology thresholds, `race.God`, toggles); on failure, print a clear message and return.

Adding a new command (what Copilot should do)
1. Export the function in `gb/commands/commands.cppm`:
   - `export void newcmd(const command_t&, GameObj&);`
2. Implement in `gb/commands/newcmd.cc` as above, with `module commands;` and `namespace GB::commands`.
3. Add the file to the `commands` target in `gb/CMakeLists.txt`.
4. Map the command string(s) in `gb/GB_server.cc::getCommands()`:
   - `{"newcmd", GB::commands::newcmd},` (and aliases if needed).
5. Use `g.out` for all messages and return early on errors.
6. Read/write via `gblib:files_shl` (e.g., `auto planet = getplanet(g.snum, g.pnum);`).

Do and don't for Copilot
- Do:
  - Use `import gblib; import std.compat;` at file top.
  - Use repository helpers and types from `gblib:*`.
  - Keep user messages consistent and helpful; end lines with `\n`.
- Don't:
  - Don't introduce exceptions, raw `new/delete`, or non-stdlib deps.
  - Don't bypass the command registry or gblib access layer.
  - Don't hardcode file paths or magic numbers—use constants/types.

Repository specifics Copilot can reference
- Command exports: `gb/commands/commands.cppm`
- Command registry: `gb/GB_server.cc` (see `getCommands()`)
- File constants: `gb/files.h` (`EXAM_FL`, etc.)
- Types and utilities: `gb/gblib-*.cppm` (notably `gblib-types.cppm`, `gblib-files_shl.cppm`, `gblib-misc.cppm`)
- Build system: `CMakeLists.txt`, `gb/CMakeLists.txt`, `cmake/compiler_setup.cmake`