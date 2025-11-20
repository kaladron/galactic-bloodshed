# AI Agent Guide for Galactic Bloodshed

## 🎯 Project Overview

**Galactic Bloodshed** is a multiplayer space empire game server written in modern C++26. This is a modernization of a classic game from the early 1990s, now using C++ Modules, CMake, and contemporary C++ practices.

### Key Characteristics
- **Language**: C++26 with C++ Modules (prefer `import std;` over `import std.compat;`)
- **Build System**: CMake with module support
- **Compiler**: LLVM/Clang with libc++
- **Architecture**: Command-based server with player actions as free functions
- **Database**: SQLite3 for persistent storage
- **Default DB path**: The code opens the DB with `sqlite3_open(PKGSTATEDIR "gb.db", ...)`. By default (CMake define) `PKGSTATEDIR` is `/usr/local/var/galactic-bloodshed/`, so the DB file is `/usr/local/var/galactic-bloodshed/gb.db` unless reconfigured.
- **Dependencies**: Minimal - SQLite3, glaze (JSON), scnlib (parsing)
- **License**: Apache-2.0

## 🔨 Building the Project

### Prerequisites
- LLVM/Clang with libc++ support
- CMake 4.0+
- SQLite3 development libraries
- Git (for dependency fetching)

### Build Commands
This project uses **CMake**, not make. The build directory is typically `build/`.

**IMPORTANT**: Always run build commands from the workspace root using `-C build` flag to avoid getting lost in the directory tree.

```bash
# Configure (first time only from workspace root)
cmake -S . -B build

# Build everything (from workspace root)
cmake --build build

# Build specific targets (from workspace root)
cmake --build build --target GB           # Main game server
cmake --build build --target makeuniv     # Universe generator
cmake --build build --target enrol        # Player enrollment
cmake --build build --target race_sqlite_test  # Database test

# Using ninja directly (from workspace root)
ninja -C build                            # Build all
ninja -C build GB                         # Build specific target

# Clean build (from workspace root)
cmake --build build --clean-first

# Run tests (from workspace root using subshell)
(cd build && ctest)                       # Run all tests
(cd build && ctest -R [test_name])       # Run specific test
(cd build && ctest --verbose)            # Run tests with verbose output
```

### Common Build Commands for AI Agents
**Always execute from `/workspaces/galactic-bloodshed` (workspace root):**

- **Full build**: `cmake --build build` or `ninja -C build`
- **Incremental build**: `cmake --build build` or `ninja -C build`
- **Build specific target**: `cmake --build build --target [target_name]` or `ninja -C build [target_name]`
- **Clean build**: `cmake --build build --clean-first`
- **Run all tests**: `(cd build && ctest)` (uses subshell to avoid changing directory)
- **Run specific test**: `(cd build && ctest -R [test_name])`
- **Run tests with verbose output**: `(cd build && ctest --verbose)`

### ⚠️ Important Notes
- **DO NOT use `make`** - This project uses CMake, not traditional makefiles
- **Stay in workspace root** - Use `cmake --build build` or `ninja -C build` to avoid cd'ing around
- Use `cmake --build build` or `ninja -C build` instead of cd'ing into build/ directory
- The build system handles C++ modules automatically
- **Prefer `ctest`** over directly running test executables for consistency and better output
- For ctest, use subshell: `(cd build && ctest)` to run from workspace root

## 🏗️ Architecture & Code Organization

### Directory Structure
```
/workspaces/galactic-bloodshed/
├── gb/                     # Core game implementation
│   ├── commands/          # Player command implementations
│   │   ├── commands.cppm  # Module interface exporting all commands
│   │   └── *.cc          # Individual command implementations
│   ├── gblib-*.cppm      # Core library module partitions
│   ├── GB_server.cc      # Main server with command dispatch
│   └── CMakeLists.txt    # Build configuration
├── docs/                  # Documentation
├── cmake/                 # CMake configuration
└── CMakeLists.txt        # Root build file
```

### Module Architecture
The codebase uses C++ Modules with the following structure:
- **`gblib`**: Core library module with partitions:
  - `gblib:types` - Game types and data structures
  - `gblib:ships` - Ship types and capabilities
  - `gblib:files_shl` - File I/O and persistence layer
  - `gblib:misc` - Utility functions
  - `gblib:tweakables` - Game configuration constants
- **`commands`**: Player command implementations module

## 📝 Coding Standards & Conventions

### File Structure Template
Every source file MUST follow this exact pattern:

```cpp
// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;  // Prefer std over std.compat for new code

module commands;  // or appropriate module partition

namespace GB::commands {
void example(const command_t& argv, GameObj& g) {
    // Implementation
}
}  // namespace GB::commands
```

### Core Patterns

#### Command Implementation Pattern
```cpp
void commandname(const command_t& argv, GameObj& g) {
    // 1. Validate scope/permissions
    if (g.level != ScopeLevel::LEVEL_SHIP && g.level != ScopeLevel::LEVEL_PLAN) {
        g.out << "Invalid scope. Must be at ship or planet.\n";
        return;
    }
    
    // 2. Parse arguments and validate
    if (argv.size() < 2) {
        g.out << "Usage: commandname <arg>\n";
        return;
    }
    
    // 3. Access current player's race (read-only)
    // g.race is already set by process_command() - no need to check for null
    // For read-only access to current player's race:
    if (g.race->some_field) {
        // Use g.race-> directly
    }
    
    // For other entities (read-only):
    const auto* star = g.entity_manager.peek_star(g.snum);
    if (!star) {
        g.out << "Star not found.\n";
        return;
    }
    
    // For modifying current player's race:
    // g.race exists (set by process_command), but need get_race() for RAII
    auto race_handle = g.entity_manager.get_race(g.player);  // No null check needed
    auto& race = *race_handle;
    race.tech += 10.5;  // Marks dirty, will auto-save
    
    // For modifying other entities:
    auto planet_handle = g.entity_manager.get_planet(g.snum, g.pnum);
    if (!planet_handle.get()) {
        g.out << "Planet not found.\n";
        return;
    }
    
    // 4. Perform game logic
    auto& planet = *planet_handle;  // Marks dirty, will auto-save
    planet.popn += 1000;
    
    // 5. Write output to player
    g.out << std::format("Success: population now {}\n", planet.popn);
    
    // 6. Auto-save happens when handles go out of scope
}
```

#### Error Handling
- **NO EXCEPTIONS** in command paths
- Use early returns with clear error messages
- Use `std::optional` for maybe-values
- Always check `.has_value()` before dereferencing

#### Output Formatting
```cpp
// All user output goes through g.out
g.out << std::format("Player {} has {} ships\n", player_id, ship_count);

// For tables, use consistent formatting
g.out << std::format("{:<15} {:>5} {:>5}\n", "Name", "Crew", "Tech");
g.out << std::format("{:<15} {:>5} {:>5}\n", name, crew, tech);
```

## 🔧 Common Tasks

### Adding a New Command

1. **Export in `gb/commands/commands.cppm`**:
   ```cpp
   export void newcommand(const command_t&, GameObj&);
   ```

2. **Create `gb/commands/newcommand.cc`** following the template above

3. **Add to `gb/CMakeLists.txt`**:
   ```cmake
   PRIVATE commands/newcommand.cc
   ```

4. **Register in `gb/GB_server.cc::getCommands()`**:
   ```cpp
   {"newcommand", GB::commands::newcommand},
   {"nc", GB::commands::newcommand},  // Optional alias
   ```

### Accessing Game Data

**IMPORTANT:** The codebase is migrating from global arrays and direct file I/O to an EntityManager-based architecture. Use the new patterns below for all new code.

#### Modern Pattern: EntityManager (Use This!)

The `EntityManager` provides centralized, RAII-based entity lifecycle management:

**Read-Only Access (peek methods - no auto-save):**
```cpp
// Read race data (no modifications, no auto-save)
const auto* race = g.entity_manager.peek_race(g.player);
if (!race) {
    g.out << "Race not found.\n";
    return;
}
g.out << std::format("Race: {}\n", race->name);

// Read star data
const auto* star = g.entity_manager.peek_star(star_id);

// Read planet data (composite key)
const auto* planet = g.entity_manager.peek_planet(star_id, planet_num);
```

**Read-Write Access (get methods - RAII with auto-save):**
```cpp
// Get entity for modification (auto-saves on scope exit)
auto race_handle = g.entity_manager.get_race(g.player);
if (!race_handle.get()) {
    g.out << "Race not found.\n";
    return;
}

// Read-only access without marking dirty
const auto& race_read = race_handle.read();
g.out << std::format("Current tech: {}\n", race_read.tech);

// Modification access (marks dirty, triggers auto-save)
auto& race = *race_handle;
race.tech += 10.5;
// Auto-saves when race_handle goes out of scope

// Explicit early save if needed
race_handle.save();
```

**Available EntityManager Methods:**
- **Races**: `peek_race(id)`, `get_race(id)`
- **Ships**: `peek_ship(id)`, `get_ship(id)`, `num_ships()`
- **Stars**: `peek_star(id)`, `get_star(id)`
- **Planets**: `peek_planet(star_id, planet_num)`, `get_planet(star_id, planet_num)`
- **Sectors**: `peek_sector(planet_id, x, y)`, `get_sector(planet_id, x, y)`
- **Commodities**: `peek_commod(id)`, `get_commod(id)`, `num_commods()`
- **Blocks**: `peek_block(id)`, `get_block(id)`
- **Power**: `peek_power(id)`, `get_power(id)`
- **Universe Data**: `peek_universe()`, `get_universe()`

**Key Benefits:**
- **RAII**: Auto-saves modified entities when handle goes out of scope
- **Caching**: Entities loaded once, reused across multiple accesses
- **Type-safe**: Compile-time checking of entity types
- **No manual persistence**: No need to call `put*()` functions

#### Legacy Pattern (Being Phased Out)

**⚠️ DEPRECATED - Do not use in new code:**

**Note:** Some executables (like `enrol.cc`) still use a mixed pattern during migration:
- `peek_*()` for read-only EntityManager access
- `get*()` + `put*()` for writes (e.g., `getstar()` returns `Star` by value, then `putstar()` to persist)
- This mixed approach will be fully replaced by EntityManager `get_*()` handles in future phases

#### Read Operations
```cpp
// Get star system
auto star = getstar(snum);

// Get planet
auto planet = getplanet(snum, pnum);

// Get ship (returns std::optional)
auto ship = getship("#123");
if (!ship) {
    g.out << "Ship not found.\n";
    return;
}

// Get sector
auto sector = getsector(planet, x, y);
```

#### Write Operations
```cpp
// Modify and persist ship
ship->fuel += 100;
putship(*ship);

// Modify and persist planet
planet.popn += 1000;
putplanet(planet, stars[snum], pnum);

// Use finish_* helpers for complex operations
finish_build_ship(sector, x, y, planet, snum, pnum, outside, level, builder);
```

**Note:** These legacy functions will be removed in Phase 6 of the database migration. All new code should use EntityManager.

### Working with GameObj Context

The `GameObj& g` parameter provides:
- `g.player` - Current player number (1-indexed)
- `g.governor` - Current governor number
- `g.race` - **Pointer to current player's race** (already populated by `process_command()`, always valid)
- `g.level` - Current scope level (UNIV/STAR/PLAN/SHIP)
- `g.snum` - Current star number
- `g.pnum` - Current planet number
- `g.shipno` - Current ship number
- `g.out` - Output stream to player
- `g.entity_manager` - **NEW:** Centralized entity access (use this instead of global arrays!)

**Key Pattern**: `g.race` is pre-populated before any command executes:
- For **read-only** access to current player's race: Use `g.race->field` directly
- For **modifications** to current player's race: Use `g.entity_manager.get_race(g.player)` for RAII (no null check needed)
- For **other players' races**: Use `peek_race(id)` or `get_race(id)` with null checks

### Writing Tests

When creating new test files, follow this essential pattern for database initialization:

```cpp
// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  // CRITICAL: Always create in-memory database BEFORE calling initialize_schema()
  Database db(":memory:");

  // Initialize database tables - this creates all required tables
  initialize_schema(db);

  // Your test logic here...
  
  std::println("Test passed!");
  return 0;
}
```

**⚠️ Critical Database Initialization Rules for Tests:**
- **ALWAYS** create `Database db(":memory:");` before calling `initialize_schema(db)`
- This creates all required tables including `tbl_ship`, `tbl_race`, `tbl_commod`, etc.
- Without this, tests will segfault when trying to access non-existent database files
- The `initialize_schema()` function creates the database schema but requires an active connection
- All working tests follow this pattern - never deviate from it
- Tests typically also need `import dallib;` in addition to `import gblib;`

## ⚠️ Critical Rules & Anti-patterns

### DO NOT:
- ❌ Use `#include` for new code (except for legacy constants from `gb/files.h`, `gb/buffers.h`)
- ❌ Use `printf`, `std::cout`, or direct console I/O
- ❌ Throw exceptions in command paths
- ❌ Use raw `new`/`delete` or manual memory management
- ❌ Add external dependencies without approval
- ❌ Hardcode file paths or magic numbers
- ❌ Create global state variables
- ❌ Bypass the gblib access layer for data persistence

### ALWAYS:
- ✅ Use `import gblib;` and prefer `import std;` over `import std.compat;`
- ✅ For tests, also add `import dallib;`
- ✅ Write all output through `g.out`
- ✅ Check `std::optional` values before use
- ✅ Use early returns with clear error messages
- ✅ Use `std::format` for string formatting
- ✅ End output lines with `\n`
- ✅ Use existing constants from `gb/files.h` and `gblib:tweakables`
- ✅ Follow the established command pattern exactly

## 🎮 Game Concepts

### Scope Levels
- `LEVEL_UNIV` - Universe level
- `LEVEL_STAR` - Star system level
- `LEVEL_PLAN` - Planet level
- `LEVEL_SHIP` - Individual ship level

### Key Game Objects
- **Race**: Player civilization with tech levels, governors, etc.
- **Star**: Star system containing planets
- **Planet**: Celestial body with sectors, population, resources
- **Ship**: Spacecraft with various types and capabilities
- **Sector**: Grid cell on a planet surface

### Ship Types & Capabilities
Ships have various abilities defined in `Shipdata` arrays:
- `ABIL_CARGO` - Cargo capacity
- `ABIL_GUNS` - Weapon systems
- `ABIL_TECH` - Required technology level
- `ABIL_COST` - Build cost
- etc.

## 📋 Checklist for Code Changes

Before submitting any code:

- [ ] File starts with `// SPDX-License-Identifier: Apache-2.0`
- [ ] Uses module imports, not `#include` headers
- [ ] All output goes through `g.out`
- [ ] Error handling uses early returns, not exceptions
- [ ] `std::optional` values are checked before use
- [ ] String formatting uses `std::format`
- [ ] No hardcoded paths or magic numbers
- [ ] Command is registered in `GB_server.cc` if applicable
- [ ] File is added to `CMakeLists.txt` if new
- [ ] Code follows the established patterns exactly

## 🔍 Quick Reference

### Common Imports Pattern
```cpp
module;
import gblib;
import std;  // Prefer std over std.compat for new code
#include "gb/files.h"  // Only if needed for constants
module commands;
```

### Command Signature
```cpp
void commandname(const command_t& argv, GameObj& g)
```

### Output Examples
```cpp
g.out << "Simple message\n";
g.out << std::format("Formatted: {} at {}\n", value, location);
g.out << std::format("{:<15} {:>5}\n", "Name", "Value");  // Table header
```

### File Path Constants
Use these from `gb/files.h`:
- `EXAM_FL` - Ship examination data
- `DATA_DIR` - Data directory path
- Others as defined

## 🐛 Debugging Tips

1. **Build Issues**: 
   - Ensure you're using LLVM/Clang with libc++
   - Always use `cmake --build build` from the workspace root using `-C build` flag
   - **Never use `make`** - this project uses CMake exclusively
2. **Module Errors**: Check module import order and partition syntax
3. **Runtime Errors**: Look for unchecked `std::optional` access
4. **Output Issues**: Verify all output goes through `g.out`
5. **Persistence Issues**: Ensure proper `put*` calls after modifications
6. **Test Failures**: 
   - Use `ctest` or `ctest --verbose` to run tests from `build/` directory
   - Individual test executables can be run directly: `./gb/race_sqlite_test`
   - Always ensure tests use `Database db(":memory:");` before `initialize_schema(db)`

## � Python Client Development

The project includes a Python client (`client/gb-client.py`) for connecting to the game server.

### Key Technologies
- **Python 3.12+** with asyncio for concurrent I/O
- **curses library** for terminal UI (split screen with output/input)
- **Client-Server Protocol (CSP)** - messages prefixed with `|`

### Phase 1 Complete (October 2025)
Phase 1 implementation achieved:
- ✅ Curses-based terminal UI with split screen
- ✅ Character-by-character input with editing (backspace, arrows, etc.)
- ✅ Async I/O with proper timeout handling
- ✅ Graceful quit/exit functionality
- ✅ ~780 lines of working Python code

### Running the Client
```bash
# Terminal 1: Start server
cd /workspaces/galactic-bloodshed/build
ninja run-gb-debug

# Terminal 2: Run client
cd /workspaces/galactic-bloodshed/client
./gb-client.py localhost 2010

# Optional: Run without curses for debugging
./gb-client.py localhost 2010 --no-curses
```

### Critical Asyncio Patterns for Client Work

1. **Use `asyncio.wait()` not `gather()` for concurrent loops**:
   - `gather()` waits for ALL tasks - wrong for input/receive loops
   - `wait(FIRST_COMPLETED)` exits when ANY task completes - correct

2. **Always add timeouts to receive loops**:
   - Use `asyncio.wait_for(reader.read(4096), timeout=0.5)`
   - Allows checking exit flags every 0.5s
   - Prevents hanging on quit/exit

3. **Let `curses.wrapper()` handle cleanup**:
   - Don't call `endwin()` manually - causes "endwin() returned ERR"
   - Wrapper automatically restores terminal on exit

## 📚 Quick Task Reference

These recipes provide step-by-step instructions for common tasks.

### Add a New Command
1. **Export in `gb/commands/commands.cppm`**:
   ```cpp
   export void foo(const command_t&, GameObj&);
   ```

2. **Create `gb/commands/foo.cc`**:
   ```cpp
   // SPDX-License-Identifier: Apache-2.0
   module;
   import gblib;
   import std;
   module commands;

   namespace GB::commands {
   void foo(const command_t& argv, GameObj& g) {
     // Validate g.level, parse argv
     // Use g.entity_manager to access entities
     // Write to g.out, return early on errors
   }
   }  // namespace GB::commands
   ```

3. **Add source file to `commands` target in `gb/CMakeLists.txt`**:
   ```cmake
   PRIVATE commands/foo.cc
   ```

4. **Wire it in `gb/GB_server.cc::getCommands()`**:
   ```cpp
   {"foo", GB::commands::foo},
   {"f", GB::commands::foo},  // Optional alias
   ```

5. **Build and test**:
   ```bash
   ninja -C build
   (cd build && ctest)
   ```

### Read from Database
Use EntityManager for all entity access:
```cpp
// Read-only access (peek methods)
const auto* race = g.entity_manager.peek_race(g.player);
const auto* star = g.entity_manager.peek_star(star_id);
const auto* planet = g.entity_manager.peek_planet(star_id, planet_num);

// Always check for null
if (!race) {
  g.out << "Race not found.\n";
  return;
}
```
Always check pointers before use. On failure: print message and return early.

**Note:** `peek_*()` methods return cached pointers from EntityManager's internal cache, so repeated calls to the same entity are efficient.

### Write to Database
Use EntityManager get methods for read-write access with RAII:
```cpp
// Get entity handle (auto-saves on scope exit)
auto race_handle = g.entity_manager.get_race(g.player);
if (!race_handle.get()) {
  g.out << "Race not found.\n";
  return;
}

// Modify entity (marks dirty)
auto& race = *race_handle;
race.tech += 10.5;
// Auto-saves when race_handle goes out of scope

// Or use explicit save if needed early
race_handle.save();
```
No need to call put* functions - RAII handles persistence automatically.

### Print Aligned Tables
Use `std::format` to build headers and rows:
```cpp
g.out << std::format("{:<15} {:>5} {:>5}\n", "Name", "Crew", "Tech");
g.out << std::format("{:<15.15} {:>5} {:>5}\n", ship_name, crew, tech);
```
Follow patterns in `gb/commands/build.cc` for column widths and alignment.

### Validate Scope and Permissions
```cpp
// Scope check
if (g.level != ScopeLevel::LEVEL_SHIP && g.level != ScopeLevel::LEVEL_PLAN) {
  g.out << "Must be at ship or planet scope.\n";
  return;
}

// Permission/capability checks
if (ship.tech < required_tech && !race.God) {
  g.out << "Insufficient technology level.\n";
  return;
}

// Toggle flags
if (!race.governor[g.governor].toggle.autoreport) {
  g.out << "Autoreport is disabled.\n";
  return;
}
```

### Add Tests
Small unit-style tests can be added alongside existing tests:
```cpp
// SPDX-License-Identifier: Apache-2.0
import dallib;
import gblib;
import std.compat;
#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  
  // Test logic with assertions
  assert(result == expected);
  
  std::println("Test passed!");
  return 0;
}
```
Wire into CTest via `gb/CMakeLists.txt`.

## 📖 Additional Resources

- [`docs/gb_FAQ.txt`](docs/gb_FAQ.txt) - Historical game documentation and FAQ

---

**Remember**: This is a legacy game being modernized. Respect existing patterns while using modern C++ features. When in doubt, follow the patterns in existing command implementations like `build.cc`, `analysis.cc`, and `autoreport.cc`.