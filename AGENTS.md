# AI Agent Guide for Galactic Bloodshed

## 🎯 Project Overview

**Galactic Bloodshed** is a multiplayer space empire game server written in modern C++26. This is a modernization of a classic game from the early 1990s, now using C++ Modules, CMake, and contemporary C++ practices.

### Key Characteristics
- **Language**: C++26 with C++ Modules (prefer `import std;` over `import std.compat;`)
- **Build System**: CMake with module support
- **Compiler**: LLVM/Clang with libc++
- **Architecture**: Command-based server with player actions as free functions (see [`ARCHITECTURE.md`](ARCHITECTURE.md) for full n-tier architecture, module layout, and data flow details)
- **Database**: SQLite3 for persistent storage
- **Default DB path**: The code opens the DB with `sqlite3_open(PKGSTATEDIR "gb.db", ...)`. By default (CMake define) `PKGSTATEDIR` is `/usr/local/var/galactic-bloodshed/`, so the DB file is `/usr/local/var/galactic-bloodshed/gb.db` unless reconfigured.
- **Dependencies**: Minimal - SQLite3, glaze (JSON), scnlib (parsing)
- **License**: Apache-2.0

## 🤖 Agent Skills

- **Commit messages**: Use `.github/skills/generate-commit-message/SKILL.md` when asked to draft a commit message or commit summary.
- **Code formatting**: Always run `clang-format -i` on modified C++ files (`.cc`, `.cppm`, `.h`, `.hpp`) before committing or pushing changes (see `.github/skills/clang-format/SKILL.md`).
- **Static analysis**: Run `./tools/tidy-changed.sh` on modified C++ files before committing (see `.github/skills/clang-tidy/SKILL.md`).
- **Atomic commit workflow**: Use `.github/skills/atomic-commit-workflow/SKILL.md` for bite-sized (~200 LOC) commits with in-commit tests and verification.
- **Command implementation**: Use `.github/skills/command-implementation/SKILL.md` when adding or migrating player commands using `CommandDescriptor`.
- **Command test matrix**: Use `.github/skills/command-test-matrix/SKILL.md` when writing 4-way command tests.
- **Required workflow**: Always inspect the current git diff first so the proposed message covers the full change set, including tests and refactors.
- **Output format**: Return commit message suggestions in markdown.

## 🧭 Workflow & Development Principles

### 1. Bite-Sized Commits (~200 LOC per Commit)
- Aim for approximately **150–250 lines of code changed per commit**.
- Keep changes atomic, focused on a single responsibility, and easy to review.
- Every commit must compile cleanly (`ninja -C build`) and pass 100% of tests (`(cd build && ctest)`).
- **In-place review fixes via interactive rebase**: When addressing code review feedback on stacked commits, use interactive rebase (`git rebase -i`) to edit and amend the relevant commit in-place rather than manually unstaging/re-staging or stacking redundant fixup commits on top.

### 2. In-Commit Testing (Never Defer Tests)
- **Every commit** that introduces new features, refactors, helper methods, or command descriptors **MUST include its unit tests in the exact same commit**.
- Never separate implementation into one commit and tests into a later commit.
- **Test Fidelity & Completeness**: Preserve all distinct test cases, assertions, and edge scenarios (e.g. multi-step sequences like friendly boarding, rejection of already-docked ships, edge-case validations). Never drop, truncate, or over-consolidate existing test cases during modernization. Preserve realistic entity state and domain setup fields (e.g., `race.mass`, `race.metabolism`, distinct player IDs for role rejection tests such as Player 1 for deity and Player 2 for mortal).
- **Preserve domain formulas & template lookups in tests**: When initializing test entities (races, ships, planets), preserve baseline template lookups and domain calculations (e.g., `Shipdata[type][ABIL_BUILD]`, `Shipnames[type]`, `race.mass`, `race.metabolism`) instead of replacing them with arbitrary hardcoded magic numbers (e.g., replacing template size calculations with `100`).

### 3. Plain English Architecture Documentation
- `ARCHITECTURE.md` is for humans and AI agents to understand how systems work conceptually.
- Explain designs in **clear, concise plain English** accompanied by high-level lifecycle diagrams.
- **Do NOT** dump large struct definitions or boilerplate code blocks into `ARCHITECTURE.md`. If a design is too complicated to explain simply in English, it is too complicated.
- Update `ARCHITECTURE.md` **incrementally** as new systems/patterns are introduced rather than deferring all documentation to the end of a project.

### 4. Context Refresh & Plan Anchoring Protocol
- AI agents will experience context compression and truncation across multi-commit workflows.
### 5. Low Cyclomatic Complexity & Domain Decomposition
- Keep domain functions and turn pipeline passes focused and small with low cyclomatic complexity ($\text{CC} \le 4$).
- Monolithic algorithms must be decomposed into composable, single-responsibility helper functions.
- When sub-steps represent discrete domain transformations (e.g. `divert_slave_tribute`, `notify_slave_revolt`, `execute_slave_revolt`), export them in the module interface and write dedicated unit tests for each. This enables isolated testing without complex multi-entity integration harness setup.

### 6. Responding to User Inquiries & Test Integrity
- When the user asks a question or makes an observation about code structure, encapsulation, or interfaces (e.g. *"are some internal to the module?"*), answer and explain the design trade-offs and testing considerations first before modifying code.
- **Never delete, drop, or truncate unit tests** in response to a structural question. If helper functions are exported to support isolated, granular unit testing, state that rationale clearly.

### 7. Code Hygiene & Style Conventions
- **Docstring & comment integrity**: Every C++ source and test file must begin with `/// \file <filename>` and `/// \brief <description>` headers immediately below the Apache-2.0 license banner. **NEVER** strip existing docstrings, file comments, stanza comments, inline explanatory comments, or test setup explanations when refactoring or migrating code.
- **TODO and disabled test retention**: You are encouraged to resolve and implement TODOs when performing the work they describe (re-enabling the associated test cases and assertions). However, **NEVER** silently delete, strip, or "clean up" TODO comments, future migration markers (e.g., `// TODO: Re-enable ... after kill_ship() migrated to EntityManager (Phase 3.7)`), or commented-out test blocks without actually implementing the fix or feature they track.
- **Cross-reference manual pages**: When defining `CommandDescriptor` metadata (scopes, min_args, AP costs, syntax strings), always check `help/<command>.md` and `help/` manual pages to ensure exact fidelity with canonical game rules.
- **Comment placement**: Explanatory comments belong at the **top of a code stanza**, not trailing at the end of statements.
- **Unused parameters**: In modern C++, if a function parameter is intentionally unused in the implementation (e.g., `argv` for no-arg commands), **omit the parameter name entirely** (`bool quit(const command_t&, GameObj& g)`) rather than annotating it with `[[maybe_unused]]`.
- **No `_impl` suffixes or forwarding wrappers**: Domain handlers in `GB::commands` are named directly after the command (`bool bless(const command_t&, GameObj&)`) and assigned directly to `.handler = &bless`. Do not create `_impl` suffixes or redundant `void bless(...)` forwarding wrappers.
- **Clean numeric literals**: Rely on implicit conversion for strong ID types (e.g., `race.Playernum = 1;`, `ctx.setup_game_obj(g, 1, 0);`) instead of verbose explicit casts (`player_t{1}`, `governor_t{0}`).
- **No Hungarian / `k` prefixes**: Use standard snake_case naming for constants and descriptors (e.g. `capital_cmd`, not `kCapitalCmd`).
- **No migration comments**: Never leave temporary migration commentary in production code (e.g. state preconditions cleanly rather than documenting past refactors).
- **Fail-fast on database corruption (no defensive try/catch on internal IDs)**: `peek_star()`, `peek_planet()`, and `peek_sectormap()` throw `EntityNotFoundError` to indicate programming bugs or data corruption. Never wrap internal/validated ID lookups (`g.snum()`, `where.snum`, `Place` parsed values) in defensive `try/catch` or null checks that silence errors; let the exceptions propagate so the server fails fast. Wrap in `try/catch` **strictly** when looking up untrusted user-supplied raw IDs (e.g. arbitrary command argument strings like `#123`).
- **Code formatting scope**: Run `clang-format -i` strictly on C++ files (`.cc`, `.cppm`, `.h`, `.hpp`). **NEVER** run `clang-format` on CMake files (`CMakeLists.txt`, `*.cmake`) or JSON data files.

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
│   ├── dal/              # Data Access Layer (Database, JsonStore, Schema)
│   ├── repositories/     # Repository pattern implementations
│   ├── services/         # Service layer (EntityManager)
│   ├── utils/            # Utility functions (rand)
│   ├── creator/          # Universe generation tools
│   ├── third_party/      # Third-party module wrappers
│   ├── gblib-*.cppm      # Core library module partitions
│   ├── GB_server.cc      # Main server with command dispatch
│   └── CMakeLists.txt    # Build configuration
├── client/                # Python client implementation
├── docs/                  # Documentation
├── cmake/                 # CMake configuration
├── data/                  # Game data files
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
  - `gblib:repositories` - Repository implementations
  - `gblib:services` - Service layer (EntityManager)
  - `gblib:rand` - Random number utilities
- **`dallib`**: Data Access Layer module (Database, JsonStore, Schema)
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
Commands use declarative metadata (`CommandDescriptor`) paired with a thin domain handler (`bool (*)(const command_t&, GameObj&)`). Preconditions, scopes, and fixed AP pre-checks/deductions are handled automatically by `dispatch_command()`.

```cpp
namespace GB::commands {

/// Handler returns true on success (triggers AP deduction), false on domain error (0 AP deducted).
bool commandname(const command_t& argv, GameObj& g) {
    // 1. Domain argument parsing and validation
    auto target = parse_target(argv[1]);
    if (!target) {
        g.out << "Invalid target specified.\n";
        return false;
    }
    
    // 2. Access entities via EntityManager (read-only peek)
    const auto& star = *g.entity_manager.peek_star(g.snum());
    
    // 3. Mutate entities via RAII handle two-step pattern
    auto planet_handle = g.entity_manager.get_planet(g.snum(), g.pnum());
    auto& planet = *planet_handle;
    planet.popn += 1000;  // Auto-saves when planet_handle goes out of scope
    
    // 4. Player output through g.out
    g.out << std::format("Success: population now {}\n", planet.popn);
    return true;
}

export constexpr CommandDescriptor commandname_cmd{
    .name = "commandname",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::fixed_star(1),
    .min_args = 2,
    .syntax = "commandname <target>",
    .description = "Perform a planetary action",
    .handler = &commandname,
};

}  // namespace GB::commands
```

#### ⚠️ CRITICAL: EntityHandle Lifetime Rule

**ALWAYS use the two-step pattern** when calling `get_*()` methods. Dereferencing directly from the return value destroys the handle immediately, causing auto-save **before** your modifications:

```cpp
// ❌ WRONG - Handle destroyed immediately, auto-save happens too early!
auto& planet = *g.entity_manager.get_planet(g.snum, g.pnum);
planet.popn += 1000;  // This change is NOT saved!

// ✅ CORRECT - Handle stays alive until scope exit
auto planet_handle = g.entity_manager.get_planet(g.snum, g.pnum);
auto& planet = *planet_handle;
planet.popn += 1000;  // Modifications happen here
// Auto-save occurs when planet_handle goes out of scope
```

**Why:** `get_*()` returns an `EntityHandle<T>` temporary. When you dereference it immediately with `*`, the temporary is destroyed at the end of that statement, triggering the destructor and auto-save. Your subsequent modifications happen on a reference to data that's already been saved, so changes are lost.

#### Error Handling
- Use early returns with clear error messages
- Use `std::optional` for maybe-values and always check `.has_value()` before dereferencing
- `EntityNotFoundError` exceptions from `peek_star()`, `peek_planet()`, and `peek_sectormap()` indicate programming errors or data corruption and should propagate for admin investigation

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

#### Entity List Iteration Pattern

Use entity list helpers when scanning collections managed by `EntityManager`:

```cpp
// Read-only iteration: prefer ::readonly(...)
for (const Race* race : RaceList::readonly(g.entity_manager)) {
  g.out << std::format("{}\n", race->name);
}

// Mutable iteration: use the writable list form
for (auto race_handle : RaceList(g.entity_manager)) {
  race_handle->tech += 1.0;
}
```

Rules:
- Use `XxxList::readonly(...)` for read-only iteration over `RaceList`, `StarList`, `PlanetList`, `CommodList`, and new `ShipList` call sites.
- Do not introduce new `const XxxList` loops for read-only access; they are transitional and being removed.
- Keep numeric loops when the code genuinely needs explicit indices for side arrays or bookkeeping.
- Keep mutable loops in the RAII handle form so auto-save behavior is preserved.

#### Container Iteration vs. Range Views Pattern

- **Direct Container Iteration**: Classes modeling collections that provide `begin()` / `end()` (such as `SectorMap`) should be iterated directly with `for (auto& item : container)`. Do **not** create duplicate member methods (e.g. `smap.sectors()` or `container.items()`) that merely wrap the default container range.
- **Differentiated Range Views**: Member range views on containers must provide distinct **filtering**, **projections**, or **alternative access dimensions**:
  - `smap.owned()`: all owned sectors (`is_owned()`)
  - `smap.owned_by(player)`: sectors belonging to a specific player
  - `smap.populated()`: sectors with population or troops (`is_populated()`)
  - `smap.populated_by(player)`: populated sectors belonging to a specific player
  - `smap.coordinates()`: 2D `(x, y)` coordinate iteration
  - `smap.indexed_sectors()`: `(Coordinates, Sector&)` pairs
  - `smap.shuffle()`: randomized turn-order traversal

#### Self-Contained Domain State Transitions

- **Point-of-Action Completeness**: Domain mutating operations (`Sector::devastate()`, `Sector::terraform()`, `Sector::colonize()`, `Planet::free_slaves()`) must leave the entity in a fully consistent, invariant-satisfying state at the point of action.
- **No End-of-Loop "Mistake Sweeps"**: Do not write catch-all loops at the end of processing passes to fix up incomplete state mutations or clear unowned entities. Fix the state transition atomically within the domain method itself.
- **Pass Rich Domain References**: Keep local colony state (`plinfo`) distinct from empire-wide state (`Race`). Pass rich domain references (`Race::gov&`, `Race&`, `const Race&`) to domain methods rather than breaking them apart into loose primitive references (`money_t&`, `unsigned long&`, `bool has_gov`).

#### Multi-Player Spatial Grid Tracking & Bitmaps

- **Dynamic Coordinate-Based Spatial Buffers**: When tracking per-sector states across multiple players on a planet grid (such as exploration, sensor visibility, or movement reaches), use `Coordinates` for all spatial coordinates and allocate dynamic buffers sized to `planet.dimensions().x * planet.dimensions().y`.
- **Per-Sector Player Bitmaps**: Use `std::vector<std::bitset<MAXPLAYERS + 1>>` to track multi-player boolean flags across grid cells to prevent cross-player state overwriting and enable fast bitwise testing/operations. Avoid fixed-size static arrays (`Sectinfo[2048]`).

#### Domain Documentation in `docs/`

- When discovering domain rules, economic models, or subsystem behavior during modernization (e.g. government centers, tax rate adjustments, climate variations, slave revolts), create human-readable markdown guides in `docs/` (e.g. `docs/governance.md`, `docs/economy.md`, `docs/planets.md`) and register them in `docs/CMakeLists.txt`.

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

// Read star data - throws EntityNotFoundError if not found
const auto& star = *g.entity_manager.peek_star(star_id);

// Read planet data (composite key) - throws EntityNotFoundError if not found
const auto& planet = *g.entity_manager.peek_planet(star_id, planet_num);

// Read sectormap data - throws EntityNotFoundError if not found
const auto* smap = g.entity_manager.peek_sectormap(star_id, planet_num);
```

**IMPORTANT:** `peek_star()`, `peek_planet()`, and `peek_sectormap()` throw `EntityNotFoundError` instead of returning nullptr. Star/planet indices are always contiguous (0 to N-1), so by the time code has a valid star/planet number, the entity must exist or data is corrupt. These exceptions represent programming errors or data corruption, not expected conditions.

**Read-Write Access (get methods - RAII with auto-save):**

For **validated/internal IDs** (e.g., `g.player`), no try/catch is needed:
```cpp
// Validated ID path: g.player is always valid
auto race_handle = g.entity_manager.get_race(g.player);
auto& race = *race_handle;
race.tech += 10.5;  // Guaranteed valid, no null check
// Auto-saves when race_handle goes out of scope
```

For **user-input IDs**, wrap in try/catch:
```cpp
// User provides a player number as command argument
player_t target_player{user_input};
try {
  auto race_handle = g.entity_manager.get_race(target_player);
  auto& race = *race_handle;
  race.tech += 10.5;
  // Auto-saves on scope exit
} catch (const EntityNotFoundError&) {
  g.out << "Player not found.\n";
  return;
}
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

**Key Pattern**: `g.race` is pre-populated before any command executes in production:
- For **read-only** access to current player's race: Use `g.race->field` directly
- For **modifications** to current player's race: Use `g.entity_manager.get_race(g.player)` for RAII (no null check needed)
- For **other entities with validated IDs**: No try/catch needed; IDs are pre-validated by game logic
- For **user-input IDs**: Wrap in try/catch to handle `EntityNotFoundError`
- **In tests**: Set `g.race = entity_manager.peek_race(g.player);` after creating GameObj to match production behavior

### Writing Tests

When creating new test files, follow this essential pattern for database initialization:

```cpp
// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  // CRITICAL: Always create in-memory database BEFORE calling initialize_schema()
  Database db(":memory:");

  // Initialize database tables - this creates all required tables
  initialize_schema(db);
  
  // Create EntityManager for accessing entities
  EntityManager em(db);
  
  // Create JsonStore for repository operations (if needed)
  JsonStore store(db);

  // Your test logic here...
  // Example: Create and save a race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.governor[0].money = 1000;
  
  RaceRepository races(store);
  races.save(race);
  
  // Create GameObj for testing commands
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);  // IMPORTANT: Set race pointer like production does
  
  // Verify with EntityManager
  const auto* saved = em.peek_race(1);
  assert(saved);
  assert(saved->governor[0].money == 1000);
  
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
- Create `EntityManager em(db)` after schema initialization for entity access
- Create `JsonStore store(db)` if you need to use repositories directly

## ⚠️ Critical Rules & Anti-patterns

### DO NOT:
- ❌ Use `#include` for new code (except for legacy constants from `gb/files.h`, `gb/buffers.h`)
- ❌ Use `printf`, `std::cout`, or direct console I/O
- ❌ Use raw `new`/`delete` or manual memory management
- ❌ Add external dependencies without approval
- ❌ Hardcode file paths or magic numbers
- ❌ Create global state variables
- ❌ Bypass the gblib access layer for data persistence
- ❌ Check for null pointers from `peek_star()`, `peek_planet()`, or `peek_sectormap()` - these throw exceptions instead
- ❌ Catch or suppress `EntityNotFoundError` on internal/validated IDs (e.g., `g.snum()`, `where.snum`) - let it fail fast on data corruption
- ❌ Drop, shorten, or consolidate away existing test cases, assertions, or explanatory comments during modernization

### ALWAYS:
- ✅ Use `import gblib;` and prefer `import std;` over `import std.compat;`
- ✅ For tests, also add `import dallib;`
- ✅ Write all output through `g.out`
- ✅ Check `std::optional` values before use
- ✅ Use early returns with clear error messages
- ✅ Use `std::format` for string formatting
- ✅ End output lines with `\n`
- ✅ Use existing constants from `gb/files.h` and `gblib:tweakables`
- ✅ Dereference `peek_star()`, `peek_planet()`, and `peek_sectormap()` results directly - they throw on not-found
- ✅ Wrap in `try/catch` only when looking up untrusted user-supplied raw IDs (e.g. parsed strings)
- ✅ Retain all docstrings, stanza comments, inline explanatory comments, and test setup explanations verbatim
- ✅ Use interactive rebase (`git rebase -i`) to amend stacked commits in-place when addressing review feedback
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
- [ ] Error handling uses early returns
- [ ] `std::optional` values are checked before use
- [ ] String formatting uses `std::format`
- [ ] No hardcoded paths or magic numbers
- [ ] Command is registered in `GB_server.cc` if applicable
- [ ] File is added to `CMakeLists.txt` if new
- [ ] Code follows the established patterns exactly
- [ ] No null checks for `peek_star()`, `peek_planet()`, or `peek_sectormap()` - these throw exceptions

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

## 🖥️ Python Client

The project includes a Python client for connecting to the game server. See [`client/AGENTS.md`](client/AGENTS.md) for detailed documentation on:
- Client setup and usage
- Asyncio patterns and best practices
- Curses UI implementation
- Debugging and development tips

## 📚 Quick Task Reference

These recipes provide step-by-step instructions for common tasks.

### Add a New Command
1. **Export descriptor in `gb/commands/commands.cppm`**:
   ```cpp
   export extern const CommandDescriptor foo_cmd;
   ```

2. **Create `gb/commands/foo.cc`**:
   ```cpp
   // SPDX-License-Identifier: Apache-2.0

   /// \file foo.cc
   /// \brief Example command implementation.

   module;
   import gblib;
   import std;
   module commands;

   namespace GB::commands {

   bool foo(const command_t& argv, GameObj& g) {
     // Domain argument parsing & entity logic via g.entity_manager
     // Return true on success (triggers AP deduction), false on domain error
     return true;
   }

   export constexpr CommandDescriptor foo_cmd{
       .name = "foo",
       .roles = {},
       .scopes = AllowedScopes::planet_only(),
       .ap = APCost::fixed_star(1),
       .min_args = 1,
       .syntax = "foo",
       .description = "Example command",
       .handler = &foo,
   };

   }  // namespace GB::commands
   ```

3. **Add source file to `commands` target in `gb/CMakeLists.txt`**:
   ```cmake
   PRIVATE commands/foo.cc
   ```

4. **Register in `gb/commands/registry.cc` (or `GB_server.cc`)**:
   ```cpp
   {"foo", &GB::commands::foo_cmd},
   ```

5. **Add 4-Way Unit Test Suite in `gb/commands/foo_test.cc`** (see `command-test-matrix` skill)

6. **Build and test**:
   ```bash
   ninja -C build
   (cd build && ctest)
   ```

### Read from Database
Use EntityManager for all entity access. All `peek_*()` methods throw `EntityNotFoundError` on failure.

For **validated/internal IDs** (e.g., `g.player` or IDs from iteration):
```cpp
// No try/catch needed - these IDs are guaranteed valid
const auto* race = g.entity_manager.peek_race(g.player);
const auto* star = g.entity_manager.peek_star(star_id);  // star_id from iteration
```

For **user-input IDs**, wrap in try/catch:
```cpp
// User provided a ship number as a command argument
try {
  const auto* ship = g.entity_manager.peek_ship(user_provided_id);
  // Use ship
} catch (const EntityNotFoundError&) {
  g.out << "Ship not found.\n";
  return;
}
```

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
import std;
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