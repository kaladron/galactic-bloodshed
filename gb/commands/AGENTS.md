# Commands Module - AI Agent Guide

## Overview

The **commands** module (`gb/commands/`) implements all player-facing commands in Galactic Bloodshed. Each command is a free function that takes user input, validates it, performs game logic, and produces output for the player.

### Module Structure

- **Module Interface**: `commands.cppm` - Exports all command functions
- **Command Implementation**: `*.cc` - One file per command (e.g., `autoreport.cc`, `bless.cc`)
- **Unit Tests**: `*_test.cc` - One test file per command or logical group

## Command Function Signature

All commands follow this pattern:

```cpp
void commandname(const command_t& argv, GameObj& g);
```

**Parameters:**
- `argv` - Command arguments (argv[0] is command name, argv[1+] are arguments)
- `g` - Game object

---

## Unit Testing Strategy

### Testing Philosophy

**Each command should have its own test file** that verifies:
1. **Database Persistence**: Changes persist after EntityManager cache clear
2. **Command Logic**: Business rules are correctly implemented
3. **Edge Cases**: Boundary conditions, error handling
4. **Invariants**: Game state remains consistent

### Test File Naming Convention

```
<command_name>_test.cc
```

Examples:
- `autoreport_test.cc` - Tests for `autoreport.cc`
- `bless_test.cc` - Tests for `bless.cc` (to be created)
- `toggle_test.cc` - Tests for `toggle.cc` (to be created)

### Test File Template

Every command test file should follow this pattern using `TestContext` and `with_standard_universe()`:

```cpp
// SPDX-License-Identifier: Apache-2.0

/// \file commandname_test.cc
/// \brief Unit tests for commandname command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_commandname_matrix() {
  TestContext ctx;
  ctx.with_standard_universe();  // Sol (0), Earth (0), Vega (1), Vega Prime (0), P1 (Federation), P2 (Klingons)

  // 1. Setup test entities via fluent builders
  shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                          .owned_by(1, 0)
                          .in_star_orbit(0)
                          .with_fuel(100.0)
                          .build();

  // 2. Setup GameObj via TestContext helper (auto-populates g.race)
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 3. 4-Way Command Matrix runner
  TestCommandMatrix(ctx, "commandname")
      .with_valid_argv({"commandname", std::format("#{}", ship_id.value)})
      .with_invalid_argv({"commandname", "#999"})
      .with_valid_scope(ScopeLevel::LEVEL_STAR)
      .with_expected_star_ap(1)
      .run_matrix(g);

  // 4. Verify domain invariants across all entities
  ctx.verify_universe_invariants();
}

void test_commandname_persistence() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Mutate via production path (auto-saves on lambda exit)
  ctx.em.mutate_planet(0, 0, [](Planet& planet) {
    planet.popn() += 500;
  });

  // Verify persistence via cache clear
  ctx.em.clear_cache();
  const auto* planet = ctx.em.peek_planet(0, 0);
  test::expect_eq(planet->popn(), 1500);
}

}  // namespace

int main() {
  test_commandname_matrix();
  test_commandname_persistence();

  std::println(std::cout, "\n✅ All commandname tests passed!");
  return 0;
}
```

### Critical Database Test Pattern

**⚠️ ALWAYS follow this pattern for command tests:**

1. **Use `TestContext` & `with_standard_universe()`**: Automatically handles in-memory database creation (`db(":memory:")`), table schema initialization (`initialize_schema(db)`), and provision of canonical solar systems with verified invariants.
2. **Use `TestShipBuilder`**: Populates test ships with canonical template parameters (`ShipTemplate`) rather than raw magic numbers.
3. **Use `ctx.setup_game_obj(g, player, gov)`**: Guarantees that `g.race` is pre-populated, preventing null pointer crashes.
4. **Mutate via `ctx.em.mutate_*()`**: Scoped monadic mutations ensure automatic persistence upon lambda exit.
5. **Clear cache before verifying disk persistence**: `ctx.em.clear_cache()` forces reload from SQLite to prove actual persistence.

---

## CMakeLists.txt Integration

Each test file needs a target in `gb/CMakeLists.txt`:

```cmake
add_executable(commandname_test commands/commandname_test.cc)
target_link_libraries(commandname_test PRIVATE dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME commandname_test COMMAND commandname_test)
```

Example for existing tests:

```cmake
# Action command tests - Phase 4.3
add_executable(autoreport_test commands/autoreport_test.cc)
target_link_libraries(autoreport_test PRIVATE dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME autoreport_test COMMAND autoreport_test)

add_executable(capital_test commands/capital_test.cc)
target_link_libraries(capital_test PRIVATE dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME capital_test COMMAND capital_test)
```

---

## Best Practices

### DO:
✅ **One test file per command** - Keeps tests focused and maintainable
✅ **Test database persistence** - Critical for EntityManager migration
✅ **Use descriptive test names** - `test_toggle_inverse_flag()` not `test1()`
✅ **Clear cache between tests** - `em.clear_cache()` ensures isolation
✅ **Create entities via Repository** - Simulates real universe creation
✅ **Follow the template** - Consistency makes codebase easier to navigate
✅ **Print success messages** - `std::println("✓ test passed")`
✅ **Assert liberally** - Catch problems early with detailed assertions

### DON'T:
❌ **Don't skip database tests** - These verify EntityManager RAII works
❌ **Don't create entities directly via EntityManager.create_*()** - Use Repository first
❌ **Don't forget `initialize_schema(db)`** - Tests will segfault without DB tables
❌ **Don't test multiple commands in one file** - Violates single responsibility
❌ **Don't use real database files** - Always use `:memory:` for unit tests
❌ **Don't skip `clear_cache()`** - You'll test cache, not persistence
❌ **Don't mix test concerns** - Separate persistence, logic, and edge case tests

---

## Testing Checklist

Before committing command migration:

- [ ] Command file migrated to EntityManager pattern
- [ ] Test file created: `<command>_test.cc`
- [ ] Database persistence test implemented
- [ ] Test added to `CMakeLists.txt`
- [ ] All tests pass: `ctest` or `ctest -R <command>_test`
- [ ] Test follows the template pattern
- [ ] Test uses `Database db(":memory:")`
- [ ] Test creates entities via Repository
- [ ] Test uses `clear_cache()` properly
- [ ] Test prints success messages

---

## Example: Complete Test File

See existing tests for reference:
- `autoreport_test.cc` - Simple toggle test
- `capital_test.cc` - Multi-entity read test
- `highlight_test.cc` - Race flag toggle test
- `motto_test.cc` - Block string modification test

---

## Questions?

Refer to:
- **Main Project Guide**: `/workspaces/galactic-bloodshed/AGENTS.md`
- **Architecture Doc**: `/workspaces/galactic-bloodshed/ARCHITECTURE.md`
- **Database Plan**: `/workspaces/galactic-bloodshed/plan-database.md`
- **Existing Tests**: Look at `*_test.cc` files in this directory
