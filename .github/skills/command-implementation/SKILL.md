---
name: command-implementation
description: 'Implement a new player-facing game command in gb/commands/. Use when adding a command, registering a command alias, validating scope/permissions, parsing argv, or producing player output. Covers the command file template, registration in GB_server.cc, CMakeLists wiring, and the standard validate/parse/act/respond structure.'
user-invocable: false
---

# Command Implementation

Every player-facing action is a free function in `gb/commands/`. Commands look uniform across the codebase. New commands must match that shape exactly.

## Signature

```cpp
void commandname(const command_t& argv, GameObj& g);
```

- `argv[0]` is the command name; `argv[1..]` are user arguments.
- `g` is the `GameObj` execution context (player, governor, scope, output stream, EntityManager).

## File Template

```cpp
// SPDX-License-Identifier: Apache-2.0

/// \file commandname.cc
/// \brief One-line description.

module;

import gblib;
import std;

module commands;

namespace GB::commands {

void commandname(const command_t& argv, GameObj& g) {
  // 1. Scope / permission checks (early return on failure)
  // 2. Argument parsing & validation
  // 3. Entity access via g.entity_manager
  // 4. Game logic
  // 5. Output via g.out
}

}  // namespace GB::commands
```

Prefer `import std;` over `import std.compat;` in new files. Use `#include` only for legacy constants from `gb/files.h` or `gb/buffers.h`.

## Standard Body Pattern

```cpp
void give(const command_t& argv, GameObj& g) {
  // 1. Scope check
  if (g.level() != ScopeLevel::LEVEL_SHIP &&
      g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "Must be at ship or planet scope.\n";
    return;
  }

  // 2. Argv validation
  if (argv.size() < 3) {
    g.out << "Usage: give <ship> <player>\n";
    return;
  }

  // 3. Read-only checks against current player's race
  if (!g.race->God && g.race->Guest) {
    g.out << "Guests cannot do this.\n";
    return;
  }

  // 4. Mutate via two-step handle pattern
  auto ship_handle = g.entity_manager.get_ship(target_ship);
  auto& ship = *ship_handle;
  ship.owner = new_owner;

  // 5. Player-visible output
  g.out << std::format("Ship {} given to player {}.\n", target_ship, new_owner);
}
```

## Output Rules

- **All** player output goes through `g.out`. Never use `printf`, `std::cout`, `std::print`.
- Use `std::format` for interpolation; never concatenate with `+`.
- End every line with `\n`.
- For tables, format header and rows with the same width spec:
  ```cpp
  g.out << std::format("{:<15} {:>5} {:>5}\n", "Name", "Crew", "Tech");
  ```

## Registering a New Command

After implementing, do all three:

1. **Export** in `gb/commands/commands.cppm`:
   ```cpp
   export void commandname(const command_t&, GameObj&);
   ```

2. **Add source** in `gb/CMakeLists.txt` under the `commands` target:
   ```cmake
   PRIVATE commands/commandname.cc
   ```

3. **Wire** in `gb/GB_server.cc::getCommands()`:
   ```cpp
   {"commandname", GB::commands::commandname},
   {"cn", GB::commands::commandname},  // optional alias
   ```

Then `ninja -C build` and `(cd build && ctest)` from the workspace root.

## Anti-Patterns

- ❌ Direct console I/O (`printf`, `std::cout`, `std::print`).
- ❌ Repository or `getstar()`/`putship()` calls — go through `g.entity_manager`.
- ❌ Inline `*g.entity_manager.get_xxx(...)` — use the two-step handle pattern.
- ❌ Null-checking `peek_star/peek_planet/peek_sectormap` results — they throw.
- ❌ Hardcoded paths or magic numbers — use `gb/files.h` constants and `gblib:tweakables`.
- ❌ `using namespace std;` or other namespace-pollution shortcuts.

## Checklist

- [ ] File starts with `// SPDX-License-Identifier: Apache-2.0`
- [ ] `module;` / `import gblib;` / `import std;` / `module commands;` header
- [ ] Wrapped in `namespace GB::commands { ... }`
- [ ] Scope and permission checks first, with early-return error messages
- [ ] Argument parsing with usage message on failure
- [ ] Entity mutations use the two-step `get_*` handle pattern
- [ ] All output through `g.out` using `std::format`
- [ ] Exported in `commands.cppm`, listed in `CMakeLists.txt`, registered in `GB_server.cc`
- [ ] Builds clean with `ninja -C build`
- [ ] Test file added under `gb/commands/<name>_test.cc` (see database-test-pattern skill)
