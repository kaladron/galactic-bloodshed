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

Specialized development skills are located in `.github/skills/` and provide comprehensive reference workflows:

- **Commit Messages**: `.github/skills/generate-commit-message/SKILL.md` — Draft repository-wide commit messages.
- **Code Formatting**: `.github/skills/clang-format/SKILL.md` — Run `clang-format -i` before committing.
- **Static Analysis**: `.github/skills/clang-tidy/SKILL.md` — Run `./tools/tidy-changed.sh` before committing.
- **Atomic Commits**: `.github/skills/atomic-commit-workflow/SKILL.md` — Bite-sized (~200 LOC) commits with in-commit tests.
- **Command Implementation**: `.github/skills/command-implementation/SKILL.md` — Authoring commands with `CommandDescriptor`.
- **Command Test Matrix**: `.github/skills/command-test-matrix/SKILL.md` — 4-way unit tests (happy path, guest, governor, scope).
- **Entity Manager Access**: `.github/skills/entity-manager-access/SKILL.md` — Scoped monadic mutations (`mutate_*`) and peeks (`with_*`, `peek_*`).
- **Entity List Iteration**: `.github/skills/entity-list-iteration/SKILL.md` — Readonly and mutable list iteration patterns.
- **Database Test Pattern**: `.github/skills/database-test-pattern/SKILL.md` — In-memory SQLite testing and persistence verification.
- **Strong ID Types**: `.github/skills/strong-id-types/SKILL.md` — Type-safe IDs (`player_t`, `shipnum_t`, `starnum_t`, `planetnum_t`), semantic metric aliases, and `PlayerVector`.
- **Entity Domain Methods**: `.github/skills/entity-domain-methods/SKILL.md` — Computed predicates, domain methods, and structured manifests on entities.
- **Repository Pattern**: `.github/skills/repository-pattern/SKILL.md` — DAL and repository implementation patterns.
- **Module File Template**: `.github/skills/module-file-template/SKILL.md` — Standard C++26 module structure and headers.
- **Domain Documentation**: `.github/skills/domain-documentation/SKILL.md` — Authoring player-facing technical guides to game mechanics and formulas.

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
- Keep domain functions and turn pipeline passes focused and small with low cyclomatic complexity ($\text{CC} \le 10$). Lower is better when it represents the right trade-off between function count and readability.
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
│   ├── dal/              # Data Access Layer (dallib)
│   ├── entities/         # Domain entities, strong IDs, collections (gb.entities)
│   ├── repositories/     # Repository pattern implementations (gb.repositories)
│   ├── services/         # Service layer (EntityManager, GameObj) (gb.services)
│   ├── turn/             # Turn simulation engine & passes (gb.turn)
│   ├── server/           # Asio server, session, auth, notification (gb.server)
│   ├── commands/         # Player command implementations (commands)
│   ├── creator/          # Universe generation tools (makeuniv)
│   ├── testing/          # Test framework & invariant verification (test)
│   ├── third_party/      # Third-party module wrappers (asio, scnlib, glaze)
│   └── CMakeLists.txt    # Build configuration
├── client/                # Python client implementation
├── docs/                  # Documentation
├── cmake/                 # CMake configuration
├── data/                  # Game data files
└── CMakeLists.txt        # Root build file
```

### Module Architecture
The codebase uses C++ Modules with the following structure:
- **`gb.entities`**: Domain models (`Race`, `Star`, `Planet`, `Ship`, `Sector`, `SectorMap`, `Universe`, `Place`, `TurnStats`), strong IDs, `PlayerVector`, `Coordinates`, `Tweakables`
- **`gb.repositories`**: Repository DAL adapters (`RaceRepository`, `ShipRepository`, `PlanetRepository`, `StarRepository`, `SectorRepository`, etc.)
- **`gb.services`**: Core game services (`EntityManager`, `GameObj`, `do_prompt`, `SessionRegistry`, `DeferredWriteScope`)
- **`gb.turn`**: Turn simulation engine (`doplanet`, `doship`, `dosector`, `doturncmd`, `do_update`, `do_segment`)
- **`gb.server`**: Server networking, session management, authentication, and notifications
- **`dallib`**: Data Access Layer module (`Database`, `JsonStore`, `Schema`)
- **`commands`**: Player command implementations module (`GB::commands`)
- **`test`**: Testing framework, fixtures, and matrix runner

## 📝 Coding Standards & Conventions

### File Structure Template
Every source file MUST follow this exact pattern:

```cpp
// SPDX-License-Identifier: Apache-2.0

module;

import gb.entities;
import gb.services;
import std;  // Prefer std over std.compat for new code

module commands;  // or appropriate module partition

namespace GB::commands {
bool example(const command_t& argv, GameObj& g) {
    // Implementation
    return true;
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
    
    // 3. Mutate entities via scoped monadic mutation (auto-saves on lambda exit)
    g.entity_manager.mutate_planet(g.snum(), g.pnum(), [](Planet& planet) {
        planet.popn += 1000;
    });
    
    // 4. Player output through g.out
    g.out << "Success: planet population updated.\n";
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

#### 🛡️ Monadic Mutation Pattern (`mutate_*`)

All entity mutations MUST go through `EntityManager::mutate_*()` methods. Calling `mutate_*` executes a mutating lambda and automatically persists changes upon lambda completion:

```cpp
// ✅ CORRECT - Scoped mutation with automatic persistence
g.entity_manager.mutate_planet(g.snum(), g.pnum(), [](Planet& planet) {
    planet.popn += 1000;  // Modifications happen here
}); // Auto-save occurs when lambda exits
```

**Why:** Internal `get_*()` handles are encapsulated as `private:` in `EntityManager` to eliminate premature save bugs and prevent holding dangling or unpersisted entity references.

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

- **Dynamic Coordinate-Based Spatial Buffers**: When tracking per-sector states across multiple players on a planet grid (such as exploration, sensor visibility, or movement reaches), use `Coordinates` for all spatial coordinates and allocate dynamic buffers sized to `planet.num_sectors()`.
- **Per-Sector Player Bitmaps**: Use `std::vector<std::bitset<MAXPLAYERS + 1>>` to track multi-player boolean flags across grid cells to prevent cross-player state overwriting and enable fast bitwise testing/operations. Avoid fixed-size static arrays (`Sectinfo[2048]`).

#### Planetary Grid Dimensions & `num_sectors()`

- **Use `num_sectors()` helper**: Prefer `planet.num_sectors()` and `smap.num_sectors()` over raw multiplication (`dimensions().x * dimensions().y`).
- **Use `smap.get_random()`**: When selecting a random sector coordinate on a world, call `smap.get_random().coords()` or `smap.get_random(rng)` instead of computing `int_rand(0, p.dimensions().x - 1)` manually.

#### Multi-Player Simulation Arrays (`PlayerVector<T, N>`)

- **Strong `player_t` Indexing**: Use `PlayerVector<T, MAXPLAYERS>` (`gblib:types`) for multi-player simulation metrics (`TurnStats`, colony arrays, power tallies) to ensure 1-based indexing, bounds safety, and Glaze JSON serialization support without raw C-arrays.

#### Domain Documentation in `docs/`

- When discovering domain rules, economic models, or subsystem behavior during modernization (e.g. government centers, tax rate adjustments, climate variations, slave revolts, planetary simulation pipelines), create human-readable markdown guides in `docs/` (e.g. `docs/governance.md`, `docs/economy.md`, `docs/planets.md`, `docs/planetary_simulation.md`) and register them in `docs/CMakeLists.txt`.

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

**Read-Write Access (monadic mutate methods - automatic persistence):**

For **validated/internal IDs** (e.g., `g.player`), no try/catch is needed:
```cpp
// Validated ID path: g.player is always valid
g.entity_manager.mutate_race(g.player, [](Race& race) {
  race.tech += 10.5;  // Guaranteed valid, no null check
});
// Auto-saves when lambda completes
```

For **user-input IDs**, wrap in try/catch:
```cpp
// User provides a player number as command argument
player_t target_player{user_input};
try {
  g.entity_manager.mutate_race(target_player, [](Race& race) {
    race.tech += 10.5;
  });
} catch (const EntityNotFoundError&) {
  g.out << "Player not found.\n";
  return;
}
```

**Available EntityManager Methods:**
- **Races**: `peek_race(id)`, `with_race(id, fn)`, `mutate_race(id, fn)`
- **Ships**: `peek_ship(id)`, `with_ship(id, fn)`, `mutate_ship(id, fn)`, `num_ships()`
- **Stars**: `peek_star(id)`, `with_star(id, fn)`, `mutate_star(id, fn)`
- **Planets**: `peek_planet(star_id, planet_num)`, `with_planet(star_id, planet_num, fn)`, `mutate_planet(star_id, planet_num, fn)`
- **SectorMaps**: `peek_sectormap(star_id, planet_num)`, `with_sectormap(star_id, planet_num, fn)`, `mutate_sectormap(star_id, planet_num, fn)`
- **Commodities**: `peek_commod(id)`, `with_commod(id, fn)`, `mutate_commod(id, fn)`, `num_commods()`
- **Blocks**: `peek_block(id)`, `with_block(id, fn)`, `mutate_block(id, fn)`
- **Power**: `peek_power(id)`, `with_power(id, fn)`, `mutate_power(id, fn)`
- **Universe Data**: `peek_universe()`, `with_universe(fn)`, `mutate_universe(fn)`
- **Server State**: `peek_server_state()`, `with_server_state(fn)`, `mutate_server_state(fn)`
- **Ship Exam**: `peek_ship_exam(type)`, `with_ship_exam(type, fn)`, `mutate_ship_exam(type, fn)`

**Key Benefits:**
- **Monadic Scoping**: Mutations are bounded to lambdas; auto-saves immediately on lambda exit
- **Encapsulated Handles**: Internal `get_*()` handles are private, preventing dangling references or premature saves
- **Caching**: Entities loaded once, reused across multiple accesses
- **Type-safe**: Compile-time checking of entity types
- **No manual persistence**: No need to call `put*()` or manual `.save()` functions

### Working with GameObj Context

The `GameObj& g` parameter provides:
- `g.player()` - Current player number (`player_t`, 1-indexed)
- `g.governor()` - Current governor number (`governor_t`)
- `g.race` - **Pointer to current player's race** (already populated by `process_command()`, always valid)
- `g.level` - Current scope level (UNIV/STAR/PLAN/SHIP)
- `g.snum()` - Current star number (`starnum_t`)
- `g.pnum()` - Current planet number (`planetnum_t`)
- `g.shipno` - Current ship number (`shipnum_t`)
- `g.out` - Output stream to player
- `g.entity_manager` - Centralized entity access service

**Key Pattern**: `g.race` is pre-populated before any command executes in production:
- For **read-only** access to current player's race: Use `g.race->field` directly.
- For **modifications** to current player's race: Use `g.entity_manager.mutate_race(g.player(), [](Race& race) { ... })`.
- For **other entities with validated IDs**: No try/catch needed; IDs are pre-validated by game logic.
- For **user-input IDs**: Wrap in try/catch to handle `EntityNotFoundError` or use `with_*`.
- **In tests**: Set `g.race = entity_manager.peek_race(g.player());` after creating GameObj to match production behavior.

### Writing Unit & Integration Tests

All tests run against an in-memory SQLite database (`Database db(":memory:");` + `initialize_schema(db);`). See [`.github/skills/database-test-pattern/SKILL.md`](.github/skills/database-test-pattern/SKILL.md) and [`.github/skills/command-test-matrix/SKILL.md`](.github/skills/command-test-matrix/SKILL.md) for full test setup guides, entity creation through repositories, and cache-clear persistence verification.

## ⚠️ Critical Rules & Anti-patterns

### DO NOT:
- ❌ Use `#include` for new code (except for legacy constants from `gb/entities/files.h`, `gb/buffers.h`)
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
- ✅ Use fine-grained module imports (`import gb.entities;`, `import gb.services;`, etc.) and prefer `import std;` over `import std.compat;`
- ✅ For tests, also add `import test;` and `import dallib;`
- ✅ Write all output through `g.out`
- ✅ Check `std::optional` values before use
- ✅ Use early returns with clear error messages
- ✅ Use `std::format` for string formatting
- ✅ End output lines with `\n`
- ✅ Use existing constants from `gb/entities/files.h` and `gblib:tweakables`
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

import gb.entities;
import gb.services;
import std;  // Prefer std over std.compat for new code

module commands;
```

### Command Signature
```cpp
bool commandname(const command_t& argv, GameObj& g)
```

### Output Examples
```cpp
g.out << "Simple message\n";
g.out << std::format("Formatted: {} at {}\n", value, location);
g.out << std::format("{:<15} {:>5}\n", "Name", "Value");  // Table header
```

### File Path Constants
Use these from `gb/entities/files.h`:
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

## 📖 Additional Resources

- [`ARCHITECTURE.md`](ARCHITECTURE.md) - Complete n-tier architecture, module layout, and data flow details
- [`docs/gb_FAQ.txt`](docs/gb_FAQ.txt) - Historical game documentation and FAQ
- `.github/skills/` - Procedural developer skills and reference workflows

---

**Remember**: This is a legacy game being modernized. Respect existing patterns while using modern C++ features. When in doubt, consult the relevant skill in `.github/skills/` or existing command implementations.