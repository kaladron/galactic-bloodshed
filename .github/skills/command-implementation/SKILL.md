---
name: command-implementation
description: 'Implement a new player-facing game command in gb/commands/. Use when adding a command, registering a command alias, validating scope/permissions, parsing argv, or producing player output. Covers CommandDescriptor metadata, handler implementation, registration, CMakeLists wiring, and the standard validate/act/respond structure.'
user-invocable: false
---

# Command Implementation

Player-facing actions are declarative `CommandDescriptor` instances paired with free-function domain handlers in `gb/commands/`.

## Architecture Overview

1. **Declarative Metadata (`CommandDescriptor`)**:
   - Declares role requirements (`god_only`, `no_guests`, `leader_only`, `star_control`).
   - Declares allowed scopes (`AllowedScopes::planet_only()`, `AllowedScopes::ship_only()`, etc.).
   - Declares AP model (`APCost::free()`, `APCost::fixed_star(cost)`, `APCost::fixed_univ(cost)`, `APCost::dynamic()`).
   - Declares argument syntax and minimum argument count.
2. **Centralized Dispatch (`dispatch_command`)**:
   - Pre-validates permissions, scope, and arguments.
   - Pre-checks sufficient AP.
   - Runs the handler (`bool (*)(const command_t& argv, GameObj& g)`).
   - Deducts AP **only** when the handler returns `true`. If the handler returns `false`, 0 AP is deducted.

## Signature

```cpp
bool commandname(const command_t& argv, GameObj& g);
```

- `argv[0]` is the command name; `argv[1..]` are user arguments (omit the `argv` name if the command takes no arguments).
- `g` is the `GameObj` execution context (player, governor, scope, output stream, EntityManager).
- Returns `true` on success (commits state and AP deduction).
- Returns `false` on domain error (aborts action, 0 AP deducted).

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

bool commandname(const command_t& argv, GameObj& g) {
  // 1. Argument parsing & domain validation (early return false on failure)
  // 2. Entity access via g.entity_manager
  // 3. Game logic / state mutation via RAII handle two-step pattern
  // 4. Player output via g.out
  return true;
}

export constexpr CommandDescriptor commandname_cmd{
    .name = "commandname",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::fixed_star(1),
    .min_args = 1,
    .syntax = "commandname <args>",
    .description = "One-line description",
    .handler = &commandname,
};

}  // namespace GB::commands
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

1. **Export descriptor** in `gb/commands/commands.cppm`:
   ```cpp
   export extern const CommandDescriptor commandname_cmd;
   ```

2. **Add source** in `gb/CMakeLists.txt` under the `commands` target:
   ```cmake
   PRIVATE commands/commandname.cc
   ```

3. **Register descriptor** in `gb/commands/registry.cc`:
   ```cpp
   reg(commandname_cmd);
   ```

4. **Add 4-way unit tests** in `gb/commands/commandname_test.cc` (see `command-test-matrix` skill).

## Anti-Patterns

- ❌ Suffixing handlers with `_impl` or writing redundant forwarding stubs — use direct `bool commandname(...)`.
- ❌ Using `[[maybe_unused]]` for unused parameters — omit the parameter name instead (`const command_t&`).
- ❌ Writing procedural permission or scope checks inside the handler — declare them in `CommandDescriptor`.
- ❌ Direct console I/O (`printf`, `std::cout`, `std::print`).
- ❌ Repository or `getstar()`/`putship()` calls — go through `g.entity_manager`.
- ❌ Inline `*g.entity_manager.get_xxx(...)` — use the two-step handle pattern.
- ❌ Null-checking `peek_star/peek_planet/peek_sectormap` results — they throw `EntityNotFoundError`.
- ❌ Hardcoded paths or magic numbers — use `gb/files.h` constants and `gblib:tweakables`.
- ❌ Hungarian `k` constant prefixes (e.g. `kCommandCmd`) — use `commandname_cmd`.

## Checklist

- [ ] File starts with `// SPDX-License-Identifier: Apache-2.0`
- [ ] `module;` / `import gblib;` / `import std;` / `module commands;` header
- [ ] Wrapped in `namespace GB::commands { ... }`
- [ ] Domain argument parsing with usage/error output and early `return false`
- [ ] Entity mutations use the two-step `get_*` handle pattern
- [ ] All output through `g.out` using `std::format`
- [ ] `CommandDescriptor` exported in `commands.cppm`, listed in `CMakeLists.txt`, registered in `registry.cc`
- [ ] 4-Way test suite added in `gb/commands/<name>_test.cc`
- [ ] Builds clean with `ninja -C build` and passes `(cd build && ctest)`
