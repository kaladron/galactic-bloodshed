# Copilot Task Playbooks

These short recipes tell Copilot how to perform common tasks in this repo.

Add a new command
1. Export in `gb/commands/commands.cppm`:
   - `export void foo(const command_t&, GameObj&);`
2. Create `gb/commands/foo.cc`:
   - SPDX header, `module;`, `import gblib;`, `import std.compat;`, `module commands;`
   - Implement `void foo(const command_t& argv, GameObj& g)` in `namespace GB::commands`.
   - Validate `g.level`, parse `argv`, use `gblib:files_shl` getters, write to `g.out`, return early on errors.
3. Add the source file to `commands` target in `gb/CMakeLists.txt`.
4. Wire it in `gb/GB_server.cc::getCommands()`:
   - `{"foo", GB::commands::foo},` (and any aliases).
5. Build and run.

Read from the database
- Prefer `gblib:files_shl` exports:
  - Stars/planets: `auto star = getstar(g.snum); auto planet = getplanet(g.snum, g.pnum);`
  - Ships: `auto ship = getship("#123");` (returns `std::optional<Ship>`)
  - Sectors: `auto sector = getsector(planet, x, y);`
- Always check `std::optional` before use. On failure: `g.out << "Not found.\n"; return;`.

Write to the database
- Modify objects in-memory, then persist with the relevant helper:
  - Ships: `putship(ship);`
  - Planet-level finishing steps: e.g., `finish_build_plan(...)` (see `commands/build.cc`).
- Avoid introducing new persistence paths. Reuse existing helpers.

Print aligned tables to the player
- Use `std::format` to build headers and rows, then stream them:
  ```c++
  g.out << std::format("{:<15} {:>5} {:>5}\n", "name", "crew", "tech");
  g.out << std::format("{:<15.15} {:>5} {:>5}\n", Shipnames[i], crew, tech);
  ```
- Follow patterns in `gb/commands/build.cc` for column widths and alignment.

Scope and permissions
- Scope: Use `g.level` to gate actions. If unsupported, print a clear line and `return;`.
- Permissions/capabilities: Follow established checks:
  - Tech level vs required tech
  - `race.God` overrides
  - Toggle flags (`race.governor[Governor].toggle.*`)
  - Ability bitmasks from ship data (see `gblib-ships.cppm` comments).

User-facing messages
- Send all messages to `g.out`.
- Keep lines terminated with `\n`.
- Prefer specific messages (what failed and what to do next).

File paths and constants
- Use `gb/files.h` constants (e.g., `EXAM_FL`) instead of string literals.
- Game-config constants live under `gblib:tweakables`.

Build and environment notes
- CMake-based; modules enabled; libc++ is required.
- External deps: SQLite3 (system), glaze, scnlib (fetched).
- Do not add new external dependencies without prior approval.

Testing
- Small unit-style tests can be added alongside (e.g., `gb/shlmisc_test.cc`) and wired into CTest.
- Prefer simple assertions for utilities; command flows are primarily integration-level.