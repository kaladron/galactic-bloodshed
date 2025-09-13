# AI Agent Guide for Galactic Bloodshed

## 🎯 Project Overview

**Galactic Bloodshed** is a multiplayer space empire game server written in modern C++26. This is a modernization of a classic game from the early 1990s, now using C++ Modules, CMake, and contemporary C++ practices.

### Key Characteristics
- **Language**: C++26 with C++ Modules (using `import std.compat`)
- **Build System**: CMake with module support
- **Compiler**: LLVM/Clang with libc++
- **Architecture**: Command-based server with player actions as free functions
- **Database**: SQLite3 for persistent storage
- **Dependencies**: Minimal - SQLite3, glaze (JSON), scnlib (parsing)
- **License**: Apache-2.0

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
import std.compat;  // or import std; if specifically needed

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
    
    // 3. Retrieve game objects using gblib:files_shl
    auto planet = getplanet(g.snum, g.pnum);
    auto ship = getship(argv[1]);  // Returns std::optional<Ship>
    if (!ship) {
        g.out << "Ship not found.\n";
        return;
    }
    
    // 4. Perform game logic
    // ...
    
    // 5. Write output to player
    g.out << std::format("Success: {}\n", result);
    
    // 6. Persist changes
    putship(*ship);
    putplanet(planet, stars[g.snum], g.pnum);
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

### Working with GameObj Context

The `GameObj& g` parameter provides:
- `g.player` - Current player number (1-indexed)
- `g.governor` - Current governor number
- `g.level` - Current scope level (UNIV/STAR/PLAN/SHIP)
- `g.snum` - Current star number
- `g.pnum` - Current planet number
- `g.shipno` - Current ship number
- `g.out` - Output stream to player

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
- ✅ Use `import gblib; import std.compat;` module imports
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
import std.compat;
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

1. **Build Issues**: Ensure you're using LLVM/Clang with libc++
2. **Module Errors**: Check module import order and partition syntax
3. **Runtime Errors**: Look for unchecked `std::optional` access
4. **Output Issues**: Verify all output goes through `g.out`
5. **Persistence Issues**: Ensure proper `put*` calls after modifications

## 📚 Additional Resources

- [`docs/COPILOT_TASKS.md`](docs/COPILOT_TASKS.md) - Task playbooks
- [`.github/copilot-instructions.md`](.github/copilot-instructions.md) - Copilot-specific guidance
- [`docs/gb_FAQ.txt`](docs/gb_FAQ.txt) - Historical game documentation

---

**Remember**: This is a legacy game being modernized. Respect existing patterns while using modern C++ features. When in doubt, follow the patterns in existing command implementations like `build.cc`, `analysis.cc`, and `autoreport.cc`.