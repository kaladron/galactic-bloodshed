---
name: n-tier-architecture
description: 'Decide which layer (DAL, Repository, Service, Application/Command) a new piece of code belongs in. Use when adding a feature that touches the database, when reviewing whether SQL or game rules are leaking across layers, or when planning a refactor that crosses persistence and game-logic boundaries. Covers layer responsibilities, allowed dependencies, and common cross-layer mistakes.'
user-invocable: false
---

# N-Tier Architecture

The codebase is organized into four layers with a strict dependency direction:

```
Commands (gb/commands/)
   │ uses services + GameObj
   ▼
Services (gb/services/, EntityManager)
   │ uses repositories + domain types
   ▼
Repositories (gb/repositories/)
   │ uses dallib (JsonStore, Database)
   ▼
DAL (gb/dal/, dallib)
   └─ owns SQLite
```

Each layer may only depend on the layers below it. Reaching upward, or skipping a layer, is the bug.

## Layer Responsibilities

### Data Access Layer (`dallib`)

Owns SQLite connections, pragmas, transactions, schema, and SQL execution. `Database`, `JsonStore`, and `initialize_schema` live here. **Must not** know any entity type, game rule, or command behavior.

### Repository Layer (`gb/repositories/`)

Owns entity-specific persistence: serialization (Glaze), table/column layout, primary keys, and entity-scoped queries (`find_by_owner`, gap-finding, etc.). **May** know what a `Ship` or `Race` is. **Must not** contain game policy (cost formulas, scoring, validation against player intent).

### Service Layer (`gb/services/`, `EntityManager`)

Owns entity identity, caching, lifecycle (RAII handles, refcounting), persistence orchestration (`flush_all`, `clear_cache`), and multi-entity operations that span repositories. Presents a stable, game-friendly API to commands and turn processing. **Must not** parse user input or write to `g.out`.

### Application Layer (`gb/commands/`, `GB_server.cc`)

Owns argv parsing, scope/permission checks, user-visible error messages, and output formatting. **Must not** touch repositories or DAL types directly. **Must** go through `EntityManager` and (where they exist) higher-level services.

## Allowed Dependencies

| From | May import | May not import |
| --- | --- | --- |
| Commands | `gblib` (GameObj, EntityManager, types, ships, etc.), `std` | `dallib`, repositories directly |
| Services | repositories, domain modules | command modules |
| Repositories | `dallib`, domain types | services, commands |
| DAL | `std`, SQLite | repositories, services, commands, domain types |

If a `import` declaration in a new file violates the table, the design is wrong — fix the layering, not the imports.

## Persistence API Guarantees

The codebase is converging on these guarantees (see `ARCHITECTURE.md`):

1. SQLite stays in DAL. No `sqlite3_*` outside `dallib`.
2. Repositories own serialization. No game rules inside.
3. `EntityManager` is the game-facing persistence interface.
4. Writable access is RAII: `get_*()` returns a handle whose lifetime drives saves.
5. Read-only access is explicit: `peek_*()` and `XxxList::readonly(...)` exist for that purpose.
6. Lookup failures are service-layer errors (`EntityNotFoundError`); commands translate them to user-visible messages.
7. Iteration semantics are consistent: readonly factories for read loops, handle iterators for mutating loops.

## Diagnosing Layer Violations

Symptoms that indicate a layer is leaking:

- A command calling a repository (`RaceRepository`) — should call `EntityManager`.
- A repository computing tax / fuel / score — game policy belongs above the repository.
- `sqlite3_exec(...)` outside `dallib` — must move into `Database`.
- A service writing to `g.out` — services do not own user output.
- `dallib` mentioning `Ship`, `Race`, etc. — those belong in `gblib` repositories.

## When You're Unsure Where Code Goes

Ask:

1. *Does it parse input or format output for the player?* → Command.
2. *Does it coordinate multiple entities or own caching/lifecycle?* → Service / EntityManager.
3. *Does it map a single entity to/from JSON or know its primary keys?* → Repository.
4. *Does it talk to SQLite directly?* → DAL.

If a function does more than one of those, split it.

## Anti-Patterns

- ❌ Commands importing `dallib` or repository headers.
- ❌ Repositories importing service headers.
- ❌ Game rules embedded in `save()`/`find()`.
- ❌ Services calling `g.out << ...` or building user-visible strings.
- ❌ DAL types referencing entity types.
- ❌ Adding a new path that bypasses `EntityManager` for "performance".

## Checklist

- [ ] New code is in the directory matching its responsibility
- [ ] Imports respect the layer dependency table
- [ ] No SQL outside `dallib`
- [ ] No game policy inside repositories
- [ ] Commands talk to `EntityManager` / services, not repositories
- [ ] Services raise typed errors (`EntityNotFoundError`); commands translate them
- [ ] User output happens only at the command layer
