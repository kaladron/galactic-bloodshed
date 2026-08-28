---
name: entity-manager-access
description: 'Access and mutate game entities (races, ships, stars, planets, sectors, commodities) through EntityManager. Use when reading or modifying any persistent game entity in commands, services, or turn processing. Covers peek_*, with_*, mutate_*, EntityNotFoundError handling, and validated vs user-input ID rules.'
user-invocable: false
---

# Entity Manager Access

`EntityManager` is the single game-facing entry point to persistent entities. Commands, services, and turn-processing code must go through it instead of touching repositories or the database directly.

## Access Patterns

| Method | Purpose | Mechanism | Auto-saves? |
| --- | --- | --- | --- |
| `peek_*(id)` | Direct read-only pointer/reference | Immediate dereference or pointer | No |
| `with_*(id, fn)` | Scoped read-only inspection | Executes `fn(const T&)` if entity exists | No |
| `mutate_*(id, fn)` | Scoped mutating transaction | Executes `fn(T&)` and persists on exit | Yes, on lambda completion |

## 🛡️ Monadic Mutation Pattern (`mutate_*`)

All entity mutations MUST go through `EntityManager::mutate_*()` methods. Calling `mutate_*` executes a mutating lambda and automatically persists changes upon lambda completion:

```cpp
// ✅ Scoped mutation with automatic persistence
g.entity_manager.mutate_planet(snum, pnum, [](Planet& planet) {
  planet.popn += 1000;  // Modifications happen here
}); // Auto-save occurs when lambda exits
```

**Why:** Internal `get_*()` handles are encapsulated as `private:` in `EntityManager` to eliminate premature save bugs and prevent holding dangling or unpersisted entity references.

## Scoped Inspection (`with_*`)

When reading an entity that might not exist (e.g. looking up an arbitrary ship ID), `with_*` executes a lambda if found and returns `std::optional<Result>`:

```cpp
auto ship_name = g.entity_manager.with_ship(target_ship_no, [](const Ship& ship) {
  return ship.name();
});
if (!ship_name) {
  g.out << "Ship not found.\n";
  return false;
}
```

## EntityNotFoundError & Direct Peeks

`peek_star`, `peek_planet`, and `peek_sectormap` throw `EntityNotFoundError` when the entity is missing. **Do not null-check** their results — dereference directly.

### Validated/internal IDs

When the ID came from game state (e.g. `g.player`, `g.snum()`, an iteration index), it is guaranteed valid. No try/catch:

```cpp
const auto& star = *g.entity_manager.peek_star(g.snum());   // safe
g.entity_manager.mutate_race(g.player, [](Race& race) {     // safe
  race.tech += 10.5;
});
```

### User-input IDs

When the ID came from untrusted user input (`argv`), wrap in `try/catch` or use `with_*`:

```cpp
try {
  g.entity_manager.mutate_ship(user_ship_id, [](Ship& ship) {
    ship.fuel() += 10.0;
  });
} catch (const EntityNotFoundError&) {
  g.out << "Ship not found.\n";
  return false;
}
```

## Available EntityManager Methods

- **Race**: `peek_race(player_t)`, `with_race(player_t, fn)`, `mutate_race(player_t, fn)`, `num_races()`
- **Ship**: `peek_ship(shipnum_t)`, `with_ship(shipnum_t, fn)`, `mutate_ship(shipnum_t, fn)`, `num_ships()`
- **Star**: `peek_star(starnum_t)`, `with_star(starnum_t, fn)`, `mutate_star(starnum_t, fn)`
- **Planet**: `peek_planet(starnum_t, planetnum_t)`, `with_planet(starnum_t, planetnum_t, fn)`, `mutate_planet(starnum_t, planetnum_t, fn)`
- **SectorMap**: `peek_sectormap(starnum_t, planetnum_t)`, `with_sectormap(starnum_t, planetnum_t, fn)`, `mutate_sectormap(starnum_t, planetnum_t, fn)`
- **Commod**: `peek_commod(commodnum_t)`, `with_commod(commodnum_t, fn)`, `mutate_commod(commodnum_t, fn)`, `num_commods()`
- **Block**: `peek_block(player_t)`, `with_block(player_t, fn)`, `mutate_block(player_t, fn)`
- **Power**: `peek_power(player_t)`, `with_power(player_t, fn)`, `mutate_power(player_t, fn)`
- **Universe**: `peek_universe()`, `with_universe(fn)`, `mutate_universe(fn)`
- **ServerState**: `peek_server_state()`, `with_server_state(fn)`, `mutate_server_state(fn)`
- **ShipExam**: `peek_ship_exam(ShipType)`, `with_ship_exam(ShipType, fn)`, `mutate_ship_exam(ShipType, fn)`

## Deferred Write Scopes (Turn Processing)

For batch simulation passes, wrap loops in `DeferredWriteScope` to accumulate mutations in memory and commit in a single atomic SQLite transaction:

```cpp
{
  DeferredWriteScope scope(g.entity_manager);
  // Multi-entity turn mutations accumulate in-memory
  for (auto race_handle : RaceList(g.entity_manager)) {
    race_handle->tech += 1.0;
  }
} // All dirty entities write to SQLite atomically upon scope exit
```

## Anti-Patterns

- ❌ Calling repositories or `getstar()`/`putstar()` etc. directly in new code.
- ❌ `if (!em.peek_star(snum))` — `peek_star` throws, never returns null.
- ❌ Wrapping internal validated IDs in defensive try/catch or null checks — let corruption fail fast.
- ❌ Storing references to entities across mutation calls.

## Checklist

- [ ] Use `peek_*` or `with_*` for read-only access
- [ ] Use `mutate_*` for state modifications
- [ ] No null checks on `peek_star/peek_planet/peek_sectormap`
- [ ] try/catch only around untrusted user-supplied raw IDs
- [ ] Tests use `DeferredWriteScope` or standard monadic mutation methods
