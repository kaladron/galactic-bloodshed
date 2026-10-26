---
name: n-tier-architecture
description: 'Decide which layer (DAL, Repository, Service, Application/Command) a new piece of code belongs in. Use when adding a feature that touches the database, when reviewing whether SQL or game rules are leaking across layers, or when planning a refactor that crosses persistence and game-logic boundaries. Covers layer responsibilities, allowed dependencies, and common cross-layer mistakes.'
user-invocable: false
---

# N-Tier Architecture

The codebase is organized into strict layers with a one-way dependency direction:

```
Commands (gb/commands/)
   │ uses presentation + mechanics + services + GameObj
   ▼
Presentation (gb/presentation/) / Turn Engine (gb/turn/)
   │ uses mechanics + services + domain entities
   ▼
Mechanics (gb/mechanics/)
   │ uses services + domain entities
   ▼
Services (gb/services/, EntityManager)
   │ uses repositories + domain entities
   ▼
Repositories (gb/repositories/)
   │ uses dallib (JsonStore, Database) + domain entities
   ▼
Entities (gb/entities/) & DAL (gb/dal/, dallib)
   └─ dallib owns SQLite
```

Each layer may only depend on the layers below it. Reaching upward, or skipping a layer, is a layering violation.

## Layer Responsibilities

### Data Access Layer (`dallib`)

Owns SQLite connections, pragmas, transactions, schema, and SQL execution. `Database`, `JsonStore`, and `initialize_schema` live here. **Must not** know any entity type, game rule, or command behavior.

### Domain Entities (`gb.entities`)

Owns pure in-memory domain models (`Race`, `Power`, `Block`, `Star`, `Planet`, `Ship`, `Sector`, `SectorMap`, `Universe`, `NewsItem`, `TelegramItem`), strong IDs, `Coordinates`, and `Tweakables`.

### Repository Layer (`gb/repositories/`, `gb.repositories`)

Owns entity-specific persistence: serialization (`gb.repositories.glaze`), table/column layout, primary keys, and entity-scoped queries (`find_by_owner`, gap-finding, etc.). **May** know what a `Ship` or `Race` is. **Must not** contain game policy (cost formulas, scoring, validation against player intent).

### Service Layer (`gb/services/`, `gb.services`, `EntityManager`)

Owns entity identity, caching, lifecycle (monadic `mutate_*`, RAII list handles, refcounting), persistence orchestration (`flush_all`, `clear_cache`, `DeferredWriteScope`), `GameObj` (`OutputBuffer`, `UiMode`), and `SessionRegistry` (transactional `SocketOutbox`). Presents a stable, game-friendly API to commands and turn processing. **Must not** parse user input.

### Shared Mechanics (`gb/mechanics/`, `gb.mechanics`)

Owns stateless multi-entity game rules (`:navigation`, `:combat`, `:construction`, `:visibility`, `:victory`) shared between interactive commands and autonomous turn passes.

### Presentation Layer (`gb/presentation/`, `gb.presentation`)

Owns dual-mode (`UiMode::ASCII` and `UiMode::JSON`) view models (`ViewModel` concept), `AnsiColor` styling, `TableBuilder` tabular formatting, and Glaze JSON serialization. **Must not** mutate game state.

### Application Layer (`gb/commands/`, `commands`)

Owns `argv` parsing, `CommandDescriptor` metadata, domain validation, and command orchestration. Emits structured output via `g.present(view_model)` or diagnostic text via `g.print(...)`. **Must not** touch repositories or DAL types directly, and **must not** use inline `std::stringstream` or third-party `tabulate`.

## Allowed Dependencies

| From | May import | May not import |
| --- | --- | --- |
| Commands | `gb.presentation`, `gb.turn`, `gb.mechanics`, `gb.services`, `gb.entities`, `std` | `dallib`, `gb.repositories` |
| Presentation | `gb.mechanics`, `gb.services`, `gb.entities`, `glaze`, `std` | `dallib`, `gb.repositories`, `commands`, `gb.turn` |
| Turn | `gb.mechanics`, `gb.services`, `gb.entities`, `std` | `dallib`, `gb.repositories`, `gb.presentation`, `commands` |
| Mechanics | `gb.services`, `gb.entities`, `std` | `dallib`, `gb.repositories`, `gb.presentation`, `gb.turn`, `commands` |
| Services | `gb.repositories` (private `.cc` only), `gb.entities`, `dallib`, `std` | `gb.mechanics`, `gb.presentation`, `gb.turn`, `commands` |
| Repositories | `dallib`, `gb.entities`, `glaze`, `std` | `gb.services`, `gb.mechanics`, `gb.presentation`, `commands` |
| DAL | `std`, SQLite | `gb.entities`, `gb.repositories`, `gb.services`, `commands` |

If an `import` declaration in a new file violates the table, the design is wrong — fix the layering, not the imports.

## Persistence API Guarantees

The codebase enforces these guarantees (see `ARCHITECTURE.md`):

1. SQLite stays in DAL. No `sqlite3_*` outside `dallib`.
2. Repositories own serialization. No game rules inside.
3. `EntityManager` is the game-facing persistence interface.
4. Writable access is scoped: `mutate_*()` executes a lambda and persists on completion; mutable `XxxList` iterators yield RAII handles.
5. Read-only access is explicit: `peek_*()`, `with_*()`, and `XxxList::readonly(...)` exist for that purpose.
6. Lookup failures are service-layer errors (`EntityNotFoundError`); commands translate them to user-visible messages when caused by untrusted user input.
7. Iteration semantics are consistent: readonly factories for read loops, handle iterators for mutating loops.

## Diagnosing Layer Violations

Symptoms that indicate a layer is leaking:

- A command calling a repository (`RaceRepository`) — should call `EntityManager`.
- A command building ASCII tables inline — should construct a `ViewModel` in `gb.presentation` and call `g.present(...)`.
- A repository computing tax / fuel / score — game policy belongs above the repository.
- `sqlite3_exec(...)` outside `dallib` — must move into `Database`.
- `dallib` mentioning `Ship`, `Race`, `NewsItem`, or `TelegramItem` — those belong in `gb.entities` and `gb.repositories`.

## When You're Unsure Where Code Goes

Ask:

1. *Does it parse input or orchestrate a player command?* → Command (`commands`).
2. *Does it format a ViewModel into ASCII tables or JSON?* → Presentation (`gb.presentation`).
3. *Does it implement shared multi-entity game rules?* → Mechanics (`gb.mechanics`).
4. *Does it coordinate entity caching, lifecycle, or notifications?* → Service (`gb.services`).
5. *Does it map a single entity to/from JSON or know its primary keys?* → Repository (`gb.repositories`).
6. *Does it talk to SQLite directly?* → DAL (`dallib`).

If a function does more than one of those, split it.

## Anti-Patterns

- ❌ Commands importing `dallib` or `gb.repositories`.
- ❌ Commands using `std::stringstream` or `tabulate` instead of `gb.presentation` ViewModels.
- ❌ Repositories importing service headers.
- ❌ Game rules embedded in `save()`/`find()`.
- ❌ DAL types referencing entity types.
- ❌ Adding a new path that bypasses `EntityManager` for "performance".

## Checklist

- [ ] New code is in the directory matching its responsibility
- [ ] Imports respect the layer dependency table
- [ ] No SQL outside `dallib`
- [ ] No game policy inside repositories
- [ ] Commands talk to `EntityManager` / services and `gb.presentation`, not repositories
- [ ] Services raise typed errors (`EntityNotFoundError`); commands translate untrusted user ID errors
- [ ] View rendering happens through `gb.presentation` (`g.present`) or `g.print`
