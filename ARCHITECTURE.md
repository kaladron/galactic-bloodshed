# Galactic Bloodshed Architecture

## Overview

Galactic Bloodshed uses a clean **n-tier architecture** with clear separation of concerns. Each layer has a single responsibility and communicates only with adjacent layers through well-defined interfaces.

The current implementation is centered on a repository-backed `EntityManager` service rather than a generic `GameDataService`. That is intentional: the codebase needs a pragmatic persistence boundary for game entities, not a large abstraction framework.

## Persistence API Guarantees

These are the architectural guarantees the project is converging toward. Some are already true in code today; the remaining cleanup work is tracked in [plan-database.md](plan-database.md).

1. **SQLite details stay in the DAL.** Raw `sqlite3_*` calls, pragmas, SQL statements, and connection lifecycle belong in `dallib` only.
2. **Repositories own serialization and table-specific persistence.** A repository may know how a `Race`, `Ship`, or `Planet` maps to storage, but it must not contain game rules.
3. **EntityManager is the game-facing persistence interface.** Commands and core game logic should load, cache, iterate, mutate, and flush entities through `EntityManager` rather than talking to repositories directly.
4. **Writable access is RAII-based.** `get_*()` returns a handle whose lifetime controls persistence, and modifications are auto-saved when the handle leaves scope.
5. **Read-only access is explicit.** `peek_*()` and readonly iterators exist so callers can traverse state without paying writable-handle overhead or implying mutation.
6. **Lookup semantics are consistent.** The target API is that entity lookup failures are treated as service-layer errors, with user-input paths translating those errors into clear command output.
7. **Iteration semantics are consistent.** Read-only loops should use explicit readonly patterns; mutable loops should use handle-based patterns.

## Layer Responsibilities

### Data Access Layer
- Owns SQLite connections, pragmas, transactions, schema creation, and SQL execution.
- Must not know game rules or command behavior.

### Repository Layer
- Owns entity-specific persistence, serialization, deserialization, and storage keys.
- May expose entity-specific queries, but not game-policy decisions.

### Service Layer
- Owns entity identity, caching, lifecycle management, persistence orchestration, and multi-entity game operations.
- Is the layer that should present a stable, game-friendly API to commands and turn processing.

### Application Layer
- Owns parsing, validation of user input, command-scoped error handling, and user-visible output.
- Must not talk to DAL or repository types directly.

## Architecture Layers

```
┌─────────────────────────────────────────────────────────┐
│                  Application Layer                       │
│              (Commands - User Interface)                 │
│                  gb/commands/*.cc                        │
└────────────────────┬────────────────────────────────────┘
                     │
                     ↓
┌─────────────────────────────────────────────────────────┐
│                   Service Layer                          │
│              (Business Logic & Coordination)             │
│                 gb/services/*.cc                         │
└────────────────────┬────────────────────────────────────┘
                     │
                     ↓
┌─────────────────────────────────────────────────────────┐
│                  Repository Layer                        │
│          (Type-Safe Data Access & Serialization)         │
│                gb/repositories/*.cc                      │
└────────────────────┬────────────────────────────────────┘
                     │
                     ↓
┌─────────────────────────────────────────────────────────┐
│              Data Access Layer (DAL)                     │
│            (Database Operations & Storage)               │
│                   gb/dal/*.cc                            │
└─────────────────────────────────────────────────────────┘
                     │
                     ↓
                 SQLite Database
```

## Cross-Cutting Concerns

**Location**: `gb/` (root directory)  
**Modules**: `gblib` module partitions (`gblib:types`, `gblib:star`, `gblib:planet`, etc.)

Cross-cutting concerns are fundamental components used across all architecture layers. These are not layers themselves but shared definitions that every layer depends on.

### Domain Entities & Types
Located in `gb/gblib-*.cppm` module partition files:

- **`gblib-types.cppm`** - Core type definitions (player_t, shipnum_t, etc.)
- **`gblib-race.cppm`** - Race entity structure
- **`gblib-ships.cppm`** - Ship entity structure and ship types
- **`gblib-star.cppm`** - Star entity structure and Star wrapper class
- **`gblib-planet.cppm`** - Planet entity structure
- **`gblib-sector.cppm`** - Sector entity structure
- **`gblib-galaxy.cppm`** - Galaxy entity structure and Galaxy wrapper class
- **`gblib-tweakables.cppm`** - Game configuration constants
- **`gblib-globals.cppm`** - Global game state (being phased out)

### Utility Functions
- **`gblib-misc.cppm`** - Miscellaneous helper functions
- **`gblib-shlmisc.cppm`** - Shell/command helper functions
- **Game Logic Modules** - `gblib-doplanet.cppm`, `gblib-doship.cppm`, etc.

### Why Root Directory?
These components are in the root `gb/` directory because they:
1. **Are not a layer** - They're foundational types, not a tier in the architecture
2. **Used by all layers** - DAL, repositories, services, and commands all need them
3. **Define the domain model** - Core game entities and types
4. **Part of gblib module** - Exported as module partitions via `gblib.cppm`

### Design Principle
Cross-cutting concerns should have **no dependencies on any architecture layer**. They define pure data structures and types that layers operate on, but don't contain business logic or data access code themselves.

---

## Module Structure

Galactic Bloodshed uses **C++26 modules** to enforce architectural boundaries. Each layer is typically a separate module:

### Standalone Modules

**These are independent modules that don't belong to gblib:**

- **`dallib`** (Data Access Layer) - `gb/dal/dallib.cppm`
  - Database, JsonStore, Schema classes
  - Only layer that knows about SQLite
  
- **`commands`** (Application Layer) - `gb/commands/commands.cppm`
  - All player commands
  - Exports command functions in `GB::commands` namespace
  
- **`session`** (Service Layer) - `gb/services/session.cppm`
  - Session class for client connections
  - NullSessionRegistry for testing
  - Network I/O with Asio (imported via `asio` module)
  
- **`gblib:sessionregistry`** (Cross-cutting) - `gb/gblib-sessionregistry.cppm`
  - SessionRegistry abstract interface (notification primitives)
  - SessionInfo struct for type-erased session metadata
  - Used by commands and services without depending on Session type

- **`notification`** (Service Layer) - `gb/services/notification.cppm`
  - Cross-player message routing with game logic
  - Respects gag toggles, star inhabitance, update_in_progress
  - Functions: d_broadcast, d_announce, d_think, d_shout, warn_player, warn_race
  
- **`asio`** (Third-party wrapper) - `gb/third_party/asio.cppm`
  - Module wrapper for Boost.Asio networking library
  - Re-exports in `asio::` namespace

### gblib Module Partitions

**The `gblib` module contains cross-cutting concerns as partitions:**

- **`gblib:types`** - Core type definitions (player_t, shipnum_t, etc.)
- **`gblib:gameobj`** - GameObj context passed to commands
- **`gblib:sessionregistry`** - SessionRegistry interface and SessionInfo struct
- **`gblib:services`** - EntityManager (core game service)
- **`gblib:repositories`** - Repository pattern implementations
- **`gblib:race`**, **`gblib:ships`**, **`gblib:star`**, **`gblib:planet`**, etc. - Entity structures
- **`gblib:tweakables`** - Game configuration constants
- **`gblib:doturncmd`** - Turn processing (do_update, do_segment, do_next_thing, ScheduleInfo)
- **`gblib:misc`**, **`gblib:shlmisc`** - Utility functions
- **Game logic partitions**: `gblib:doplanet`, `gblib:doship`, `gblib:fire`, etc.

### Module Dependencies

```
commands --> gblib (for GameObj, EntityManager, SessionRegistry interface)
         --> notification (for cross-player messaging)
         --> session (only for who/emulate commands that need Session&)

notification --> gblib (for EntityManager, SessionRegistry interface, types)

session --> gblib (for types, SessionRegistry interface)
        --> asio (for networking)

gblib:sessionregistry --> gblib:types (for player_t, governor_t, etc.)

gblib:services --> dallib (for Database)
gblib:repositories --> dallib (for JsonStore)

dallib --> (no dependencies, just SQLite)
```

### Why This Structure?

1. **`dallib` is standalone** - It's the foundation; no other modules depend on internal DAL types
2. **`commands` is standalone** - Application layer imports what it needs from service/core layers
3. **`session` is standalone** - Concrete Session class with Asio networking is isolated from game logic
4. **`gblib:sessionregistry` is a partition** - Abstract interface for notifications lives in gblib so commands don't need to import session module
5. **`gblib` contains shared types** - Everything else uses these fundamental types
6. **Clear boundaries** - Module imports enforce architectural constraints at compile time

---

## Layer Details

### Layer 1: Data Access Layer (DAL)
**Location**: `gb/dal/`  
**Module**: `dallib` (standalone module)

The DAL is the **only** layer that knows about SQLite or any database implementation details.

#### Responsibilities
- Manage database connections
- Execute raw SQL queries
- Handle transactions
- Provide generic JSON storage interface
- Manage database schema

#### Key Components

**`Database` Class**
```cpp
export class Database {
public:
  Database(const std::string& path = ":memory:");
  ~Database();
  
  void begin_transaction();
  void commit();
  void rollback();
};
```
- Encapsulates SQLite connection
- Handles connection lifecycle
- Provides transaction support
- No business logic

**`JsonStore` Class**
```cpp
export class JsonStore {
public:
  JsonStore(Database& database);
  
  bool store(const std::string& table, int id, const std::string& json);
  std::optional<std::string> retrieve(const std::string& table, int id);
  bool remove(const std::string& table, int id);
  std::vector<int> list_ids(const std::string& table);
  int find_next_available_id(const std::string& table);
};
```
- Generic CRUD operations for JSON data
- Table-agnostic storage interface
- Gap-finding for ID allocation
- Error handling

**Schema Management**
```cpp
export void initialize_schema(Database& db);
```
- Creates all database tables
- Sets up indexes
- Configures SQLite pragmas

#### Design Principles
- **No business logic**: Pure data storage operations
- **Generic operations**: Works with any JSON data
- **Single responsibility**: Only database access
- **No type knowledge**: Doesn't know about Race, Ship, etc.

---

### Layer 2: Repository Layer
**Location**: `gb/repositories/`  
**Module**: `gblib:repositories`

Repositories provide type-safe access to game entities and handle JSON serialization.

#### Responsibilities
- Serialize/deserialize game entities to/from JSON
- Provide type-safe CRUD operations
- Manage entity-specific queries
- Handle data validation
- Abstract storage details from business logic

#### Key Components

**Base Repository Template**
```cpp
template<typename T>
class Repository {
protected:
  JsonStore& store;
  std::string table_name;
  
  virtual std::optional<std::string> serialize(const T& entity) = 0;
  virtual std::optional<T> deserialize(const std::string& json) = 0;
  
public:
  Repository(JsonStore& js, const std::string& table);
  
  bool save(int id, const T& entity);
  std::optional<T> find(int id);
  bool remove(int id);
  int next_available_id();
};
```

**Specific Repositories**

Each game entity type has its own repository:

- **`RaceRepository`**: Player races
- **`ShipRepository`**: Spacecraft
- **`PlanetRepository`**: Planets
- **`StarRepository`**: Star systems
- **`SectorRepository`**: Planet surface sectors
- **`CommodRepository`**: Commodity market
- **`BlockRepository`**: Communication blocks
- **`PowerRepository`**: Power reports

**Example: ShipRepository**
```cpp
export class ShipRepository : public Repository<Ship> {
public:
  ShipRepository(JsonStore& store);
  
  // Standard operations
  std::optional<Ship> find_by_number(shipnum_t num);
  bool save_ship(const Ship& ship);
  bool remove_ship(shipnum_t num);
  
  // Ship-specific operations
  shipnum_t get_next_ship_number();
  shipnum_t count_all_ships();
  std::vector<Ship> find_by_owner(player_t owner);
  
protected:
  std::optional<std::string> serialize(const Ship& ship) override;
  std::optional<Ship> deserialize(const std::string& json) override;
};
```

#### Design Principles
- **Type safety**: Strong typing for all operations
- **Encapsulation**: Hides JSON/database details
- **Single entity focus**: Each repository handles one entity type
- **No business logic**: Pure data access
- **Dependency injection**: Receives `JsonStore` reference

#### JSON Serialization
Repositories use **Glaze** library for JSON serialization:
```cpp
// Glaze reflection defines the JSON mapping
namespace glz {
template<>
struct meta<Ship> {
  using T = Ship;
  static constexpr auto value = object(
    "owner", &T::owner,
    "shipnum", &T::shipnum,
    "fuel", &T::fuel,
    // ... all fields
  );
};
}
```

---

### Layer 3: Service Layer
**Location**: `gb/services/`  
**Modules**: 
- `gblib:services` - Core game service (EntityManager)
- `session` - Session management (standalone module)

Services contain business logic and coordinate operations across multiple repositories.

#### Responsibilities
- Implement game rules and business logic
- Coordinate multi-entity operations
- Enforce game constraints
- Provide high-level game operations
- Transaction management for complex operations

#### Key Component: EntityManager

```cpp
export class EntityManager {
public:
  explicit EntityManager(Database& database);

  // Writable access with RAII persistence
  EntityHandle<Race> get_race(player_t player);
  EntityHandle<Ship> get_ship(shipnum_t num);
  EntityHandle<Planet> get_planet(starnum_t star, planetnum_t pnum);
  EntityHandle<Star> get_star(starnum_t num);

  // Read-only access
  const Race* peek_race(player_t player);
  const Ship* peek_ship(shipnum_t num);
  const Planet* peek_planet(starnum_t star, planetnum_t pnum);
  const Star* peek_star(starnum_t num);

  // Batch persistence / cache lifecycle
  void flush_all();
  void clear_cache();
  player_t num_races();
  shipnum_t num_ships();
};
```

`EntityManager` is the practical service boundary for the game. It coordinates repositories, caches loaded entities, provides RAII handles for mutation, and exposes read-only access for inspection and iteration.

#### RAII Entity Access

```cpp
auto ship_handle = g.entity_manager.get_ship(shipno);
auto& ship = *ship_handle;
ship.fuel() += 10.0;
// Auto-save happens when ship_handle goes out of scope
```

#### Read-Only Access

```cpp
const auto* race = g.entity_manager.peek_race(g.player);
g.out << std::format("Race: {}\n", race->name);
```

#### Service Layer Responsibilities In Practice
- Coordinate repositories behind a game-specific API.
- Preserve identity/caching guarantees for loaded entities.
- Support batch flushing after turn processing.
- Host game-facing persistence operations like entity deletion, news/telegram posting, and other multi-entity actions.
- Avoid exposing DAL internals upward.

#### Design Principles
- **Business logic centralization**: All game rules in one place
- **Transaction management**: Ensures data consistency
- **Coordination**: Orchestrates multiple repositories
- **No direct database access**: Only uses repositories
- **Domain-driven**: Methods reflect game concepts

---

### Layer 4: Application Layer (Commands)
**Location**: `gb/commands/`  
**Module**: `commands` (standalone module)

Commands handle user interaction and translate user input into service calls.

#### Responsibilities
- Parse user input
- Validate command arguments
- Call service layer methods
- Format output for users
- Handle command-specific errors

#### Structure

**GameObj Context**
```cpp
struct GameObj {
  player_t player;        // Current player
  governor_t governor;    // Current governor
  const Race* race;       // Current player's race (set by process_command, always valid)
  ScopeLevel level;       // Current scope level
  starnum_t snum;         // Current star
  planetnum_t pnum;       // Current planet
  shipnum_t shipno;       // Current ship
  
  EntityManager& entity_manager;  // Service layer access
  std::ostream& out;      // Output stream
};
```

**Entity Access Pattern**: All EntityManager methods throw `EntityNotFoundError` on failure:
- **Validated IDs** (e.g., `g.player`, IDs from iteration): No try/catch needed.
- **User-input IDs**: Wrap in try/catch to handle `EntityNotFoundError`.
- **Read-only checks**: Use `g.race->field` directly (always valid in production).
- **Modifications**: Use `g.entity_manager.get_race(g.player)` for RAII (no null check needed for validated IDs).
- **In tests**: Set `g.race = entity_manager.peek_race(g.player);` after creating GameObj.

**Command Pattern**
```cpp
namespace GB::commands {

void examine(const command_t& argv, GameObj& g) {
  // 1. Validate scope and permissions
  if (g.level != ScopeLevel::LEVEL_SHIP) {
    g.out << "Must be scoped to a ship.\n";
    return;
  }
  
  // 2. Parse arguments
  if (argv.size() < 2) {
    g.out << "Usage: examine <ship>\n";
    return;
  }
  
  // 3. Call service layer
  auto shipno = string_to_shipnum(argv[1]);
  const auto* ship = g.entity_manager.peek_ship(shipno);
  if (!ship) {
    g.out << "Ship not found.\n";
    return;
  }
  
  // 4. Format and display output
  g.out << std::format("Ship #{}: {}\n", ship->number(), ship->name());
  g.out << std::format("Owner: {}\n", ship->owner());
  g.out << std::format("Fuel: {}\n", ship->fuel());
  // ... more output
}

} // namespace GB::commands
```

#### Design Principles
- **Thin layer**: Minimal logic, mostly I/O
- **Service delegation**: All data operations via service
- **User-focused**: Output formatted for humans
- **Early returns**: Fail fast with clear messages
- **No direct data access**: Never touch repositories or DAL

---

## Data Flow Examples

### Simple Read Operation: Get Ship

```
Command (examine.cc)
  ↓ g.entity_manager.peek_ship(shipnum)
Service (EntityManager)
    ↓ ships.find_by_number(shipnum)
Repository (ShipRepository)
    ↓ store.retrieve("tbl_ship", shipnum)
    ↓ deserialize(json)
DAL (JsonStore)
    ↓ SELECT ship_data FROM tbl_ship WHERE ship_id = ?
Database (SQLite)
```

### Simple Write Operation: Save Planet

```
Command (build.cc)
  ↓ planet_handle = g.entity_manager.get_planet(star, pnum)
Service (EntityManager)
  ↓ planets.find_by_location(star, pnum)
  ↓ [caller mutates *planet_handle]
  ↓ [handle destructor triggers save]
Repository (PlanetRepository)
    ↓ serialize(planet)
    ↓ store.store("tbl_planet", id, json)
DAL (JsonStore)
    ↓ REPLACE INTO tbl_planet VALUES (?, ?, ?, ?)
Database (SQLite)
```

### Complex Operation: Build Ship

```
Command (build.cc)
  ↓ g.entity_manager.create_ship(init_data)
Service (EntityManager)
    ↓ [Check tech requirements]
  ↓ [Create ship object]
  ↓ ships.next_ship_number()
  ↓ ships.save(new_ship)
  ↓ planets.find_by_location(...)
    ↓ [Deduct resources from planet]
  ↓ [planet handle persists changes]
Repository Layer
    ↓ [Multiple repository operations]
DAL (JsonStore)
  ↓ [Multiple SQL statements]
Database (SQLite)
```

---

## Module Organization

### Module Hierarchy

```
gblib (primary module)
├── gblib:types          - Core game types (Ship, Planet, Race, etc.)
├── gblib:dal            - Database access layer
├── gblib:repositories   - Repository implementations
├── gblib:services       - Business logic services
├── gblib:commands       - (Separate module for commands)
└── gblib:*              - Other game systems (combat, movement, etc.)
```

### Export Philosophy

**What to Export:**
- Public interfaces users of the layer need
- Types required by public interfaces
- Factory functions for creating objects

**What NOT to Export:**
- Internal implementation details
- Helper functions
- Database connection objects
- JSON serialization internals

**Example Module Interface:**

```cpp
// gblib-repositories.cppm
export module gblib:repositories;

import :dal;
import :types;

// Export the repository classes
export class RaceRepository { /* ... */ };
export class ShipRepository { /* ... */ };
// ... other repositories

// Do NOT export:
// - Glaze reflection (internal detail)
// - Helper functions like serialize/deserialize
// - JsonStore (DAL concern)
```

---

## Dependency Injection

### Initialization Pattern

```cpp
// In main() or initialization code
Database db(PKGSTATEDIR "gb.db");
initialize_schema(db);

// Create service boundary
EntityManager entity_manager(db);

// Commands receive EntityManager via GameObj
GameObj game_context{
  .player = current_player,
  .entity_manager = entity_manager,
  .out = player_output_stream
};

// Execute command
GB::commands::examine(command_args, game_context);
```

### Benefits
- **No global state**: All dependencies explicit
- **Testability**: Easy to mock any layer
- **Flexibility**: Can swap implementations
- **Thread safety**: Each connection independent

---

## Testing Strategy

### Test Pyramid

```
         /\
        /  \       Command Tests (few)
       /    \      - Integration tests
      /      \     - Use real service
     /--------\    
    /          \   Service Tests (some)
   /            \  - Mock repositories
  /              \ - Business logic focus
 /________________\
Repository/DAL Tests  (many)
- Unit tests
- In-memory database
- Fast and isolated
```

### Layer-Specific Testing

**DAL Tests**
```cpp
// Tests use in-memory database
Database db(":memory:");
initialize_schema(db);
JsonStore store(db);

// Test basic operations
store.store("test_table", 1, R"({"field": "value"})");
auto result = store.retrieve("test_table", 1);
assert(result.has_value());
```

**Repository Tests**
```cpp
// Tests use in-memory database
Database db(":memory:");
initialize_schema(db);
JsonStore store(db);
ShipRepository repo(store);

Ship ship = create_test_ship();
repo.save_ship(ship);

auto retrieved = repo.find_by_number(ship.shipnum);
assert(retrieved.has_value());
assert(retrieved->owner == ship.owner);
```

**Service Tests**
```cpp
Database db(":memory:");
initialize_schema(db);
EntityManager em(db);

// Test service-layer behavior
const auto* result = em.peek_ship(123);
assert(result == nullptr || result->number() == 123);
```

**Command Tests**
```cpp
// Integration test with real service
Database db(":memory:");
initialize_schema(db);
EntityManager em(db);

std::ostringstream output;
GameObj g{.player = 1, .entity_manager = em, .out = output};

GB::commands::examine({"examine", "123"}, g);

assert(output.str().contains("Ship #123"));
```

---

## Design Principles

### Single Responsibility Principle
Each layer and class has one clear purpose:
- **DAL**: Database operations only
- **Repositories**: Entity persistence only
- **Services**: Business logic only
- **Commands**: User interaction only

### Dependency Inversion
High-level modules don't depend on low-level modules:
- Commands depend on services (abstractions)
- Services depend on repositories (abstractions)
- Repositories depend on DAL (abstractions)
- No layer knows implementation details of layers below

### Open/Closed Principle
Easy to extend without modifying:
- New repositories added without changing DAL
- New services added without changing repositories
- New commands added without changing services

### Interface Segregation
Clients only depend on what they use:
- Commands only see service interface
- Services only see repository interface
- Repositories only see DAL interface

---

## Benefits of This Architecture

### Maintainability
- **Clear structure**: Easy to find code
- **Isolated changes**: Modifications don't ripple
- **Consistent patterns**: Same approach everywhere

### Testability
- **Layer isolation**: Test each layer independently
- **Mock support**: Easy to create test doubles
- **Fast tests**: In-memory database for speed

### Flexibility
- **Pluggable storage**: Can swap SQLite for PostgreSQL
- **Format changes**: JSON serialization isolated
- **Feature addition**: Clear where new code goes

### Understandability
- **Clear boundaries**: Each layer has defined role
- **Predictable flow**: Data flows through layers
- **Domain-driven**: Code reflects game concepts

### Type Safety
- **Compile-time checks**: Wrong types caught early
- **Strong interfaces**: Clear contracts between layers
- **No stringly-typed code**: IDs are proper types

---

## Anti-Patterns to Avoid

### ❌ Don't Skip Layers
```cpp
// BAD: Command directly accessing database
void command(const command_t& argv, GameObj& g) {
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(dbconn, "SELECT ...", ...);  // NO!
}

// GOOD: Command uses service
void command(const command_t& argv, GameObj& g) {
  const auto* ship = g.entity_manager.peek_ship(shipnum);  // YES!
}
```

### ❌ Don't Put Business Logic in Repositories
```cpp
// BAD: Repository contains game rules
class ShipRepository {
  bool can_build_ship(const Race& race) {  // NO!
    return race.tech >= 10;
  }
};

// GOOD: Service contains game rules
class EntityPolicyService {
  bool can_build_ship(const Race& race) {  // YES!
    return race.tech >= 10;
  }
};

// In current code, this kind of logic may live in free functions or service
// helpers that operate on EntityManager-backed entities. The rule is the same:
// keep policy out of repositories.
```

### ❌ Don't Use Global State
```cpp
// BAD: Global database connection
extern sqlite3* dbconn;  // NO!

// GOOD: Dependency injection
class Repository {
  JsonStore& store;  // YES!
};
```

### ❌ Don't Mix Concerns
```cpp
// BAD: Command contains database code
void command(const command_t& argv, GameObj& g) {
  Ship ship;
  // ... database access
  // ... business logic
  // ... output formatting
  // All mixed together - NO!
}

// GOOD: Separated concerns
void command(const command_t& argv, GameObj& g) {
  const auto* ship = g.entity_manager.peek_ship(num);  // Data access
  bool can_do = check_rules(*ship);                   // Business logic
  g.out << format_ship(ship);               // Presentation
}
```

## Iterator Cleanup Direction

The persistence API is intentionally moving toward two distinct iteration modes:

1. **Readonly iteration** for reporting, scans, and validation.
2. **Writable iteration** for mutation with RAII persistence.

### Target Patterns

```cpp
for (const Race* race : RaceList::readonly(entity_manager)) {
  // Read-only traversal
}

for (auto race_handle : RaceList{entity_manager}) {
  race_handle->tech += 1.0;
}
```

### Why This Matters
- It makes mutation visible at the call site.
- It avoids accidental writable-handle use in read-only loops.
- It gives the service layer a more coherent, ORM-like interface without hiding too much behavior.

## What This Is Not

This architecture is not trying to become a generic ORM.

- There is no goal of a database-agnostic query DSL.
- There is no goal of runtime mapping metadata or automatic relationship loading.
- The goal is a strong game-specific persistence API built around repositories, entity identity, caching, and RAII saves.

---

## File Structure

```
gb/
├── dal/
│   ├── dallib.cppm               # DAL module interface (standalone)
│   ├── database.cc              # Database connection management
│   ├── json_store.cc            # Generic JSON storage
│   └── schema.cc                # Schema initialization
│
├── repositories/
│   ├── gblib-repositories.cppm  # Repository module partition
│   ├── race_repository.cc       # Race entity persistence
│   ├── ship_repository.cc       # Ship entity persistence
│   ├── planet_repository.cc     # Planet entity persistence
│   ├── star_repository.cc       # Star entity persistence
│   ├── sector_repository.cc     # Sector entity persistence
│   ├── commod_repository.cc     # Commodity persistence
│   ├── block_repository.cc      # Communication block persistence
│   └── power_repository.cc      # Power report persistence
│
├── services/
│   ├── gblib-services.cppm      # Service module partition
│   ├── entity_manager.cc        # EntityManager service
│   ├── session.cppm             # Session module interface (standalone)
│   ├── session.cc               # Session implementation
│   ├── notification.cppm        # Notification module interface (standalone)
│   └── notification.cc          # Cross-player messaging implementation
│
├── commands/
│   ├── commands.cppm            # Command module interface (standalone)
│   └── *.cc                     # Individual command implementations
│
├── third_party/
│   ├── asio.cppm                # Asio module wrapper (standalone)
│   ├── scnlib.cppm              # scnlib module wrapper
│   └── glaze_json.cppm          # Glaze JSON module wrapper
│
├── tests/
│   ├── dal_tests/               # DAL unit tests
│   ├── repository_tests/        # Repository unit tests
│   ├── service_tests/           # Service tests
│   └── command_tests/           # Integration tests
│
└── [other game systems]/
    └── ...
```

---

## Summary

This n-tier architecture provides:

1. **Clear Separation**: Each layer has a single, well-defined responsibility
2. **Maintainability**: Easy to understand, modify, and extend
3. **Testability**: Each layer can be tested independently
4. **Flexibility**: Easy to swap implementations or add features
5. **Type Safety**: Strong typing throughout the stack
6. **No Global State**: All dependencies explicitly managed

The architecture follows SOLID principles and provides a clean, professional structure that scales well as the codebase grows.
