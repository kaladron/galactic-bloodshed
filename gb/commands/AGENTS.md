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

Every command test file should follow this pattern:

```cpp
// SPDX-License-Identifier: Apache-2.0

/// \file commandname_test.cc
/// \brief Unit tests for commandname command

import dallib;
import gblib;
import commands;
import std.compat;

#include <cassert>

// Test 1: Database persistence
void test_commandname_persistence() {
  // 1. Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // 2. Create test entities via Repository (simulates universe creation)
  JsonStore store(db);
  // ... create entities ...
  // ... save via repository ...

  // 3. Verify initial state via EntityManager
  em.clear_cache();
  {
    const auto* entity = em.peek_entity(id);
    assert(entity);
    assert(entity->field == initial_value);
  }

  // 4. Simulate command execution via EntityManager
  {
    auto entity_handle = em.get_entity(id);
    assert(entity_handle.get());
    auto& entity = *entity_handle;
    entity.field = new_value;
    // Auto-saves on scope exit
  }

  // 5. Verify changes persisted after cache clear
  em.clear_cache();
  const auto* final_entity = em.peek_entity(id);
  assert(final_entity);
  assert(final_entity->field == new_value);

  std::println("✓ commandname persistence test passed");
}

// Test 2: Command logic (specific business rules)
void test_commandname_logic() {
  // Test specific command behavior, validation, etc.
  std::println("✓ commandname logic test passed");
}

// Test 3: Edge cases
void test_commandname_edge_cases() {
  // Test boundary conditions, error handling
  std::println("✓ commandname edge cases test passed");
}

int main() {
  test_commandname_persistence();
  test_commandname_logic();
  test_commandname_edge_cases();

  std::println("\n✅ All commandname tests passed!");
  return 0;
}
```

### Critical Database Test Pattern

**⚠️ ALWAYS follow this pattern for database tests:**

```cpp
// 1. Create in-memory database BEFORE initialize_schema()
Database db(":memory:");
initialize_schema(db);
EntityManager em(db);

// 2. Create entities via Repository (not EntityManager!)
JsonStore store(db);
SomeRepository repo(store);
Entity entity{};
// ... initialize entity ...
repo.save(entity);

// 3. Clear cache before verifying initial state
em.clear_cache();

// 4. Use EntityManager for modifications
auto handle = em.get_entity(id);
auto& entity = *handle;
entity.field = new_value;
// Auto-saves on scope exit

// 5. Clear cache before final verification
em.clear_cache();
const auto* result = em.peek_entity(id);
assert(result->field == new_value);
```

**Why this pattern?**
- Creates entities in DB (not just in cache)
- Tests actual persistence, not just in-memory changes
- Simulates real game flow: universe creation → command execution → verification
- `clear_cache()` forces reload from database

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
