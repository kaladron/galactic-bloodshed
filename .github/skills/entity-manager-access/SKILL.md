---
name: entity-manager-access
description: 'Access game entities (races, ships, stars, planets, sectors, commodities) through EntityManager. Use when reading or modifying any persistent game entity in commands, services, or turn processing. Covers peek_* vs get_*, the two-step RAII handle pattern, EntityNotFoundError handling, and validated vs user-input ID rules.'
user-invocable: false
---

# Entity Manager Access

`EntityManager` is the single game-facing entry point to persistent entities. Commands, services, and turn-processing code must go through it instead of touching repositories or the database directly.

## Two Access Modes

| Method | Returns | Mutates? | Auto-saves? |
| --- | --- | --- | --- |
| `peek_*(id)` | `const T*` (or throws) | No | No |
| `get_*(id)` | `EntityHandle<T>` | Yes | Yes, on handle scope exit |

Use `peek_*` for read-only access. Use `get_*` only when you intend to modify.

## CRITICAL: Two-Step Handle Pattern

`get_*()` returns a temporary `EntityHandle<T>`. Dereferencing it inline destroys the handle immediately, triggering auto-save **before** your modifications.

```cpp
// ❌ WRONG — handle destroyed at end of statement, modifications lost
auto& planet = *g.entity_manager.get_planet(snum, pnum);
planet.popn += 1000;  // NOT SAVED

// ✅ CORRECT — handle outlives the modifications
auto planet_handle = g.entity_manager.get_planet(snum, pnum);
auto& planet = *planet_handle;
planet.popn += 1000;
// Auto-save fires when planet_handle leaves scope
```

This is the single most common mistake. Always bind the handle to a named local first.

## EntityNotFoundError

`peek_star`, `peek_planet`, `peek_sectormap`, `peek_race`, `peek_ship`, `get_race`, `get_ship` (and other lookups) throw `EntityNotFoundError` when the entity is missing. **Do not null-check** their results — dereference directly.

### Validated/internal IDs

When the ID came from game state (e.g. `g.player`, `g.snum`, an iteration index), it is guaranteed valid. No try/catch:

```cpp
const auto* star = g.entity_manager.peek_star(g.snum);   // safe
auto race_handle = g.entity_manager.get_race(g.player);  // safe
auto& race = *race_handle;
race.tech += 10.5;
```

### User-input IDs

When the ID came from `argv`, wrap in try/catch and translate to a player-visible message:

```cpp
try {
  const auto* ship = g.entity_manager.peek_ship(user_provided_id);
  // use ship...
} catch (const EntityNotFoundError&) {
  g.out << "Ship not found.\n";
  return;
}
```

## g.race Is Pre-Populated

`process_command()` sets `g.race` before any command runs. For the **current player's race**:

- Read-only: use `g.race->field` directly.
- Mutate: `auto race_handle = g.entity_manager.get_race(g.player);` (no null check needed).

In tests, the test harness must set `g.race = em.peek_race(g.player);` after constructing `GameObj`, mirroring production.

## Available Methods

- Race: `peek_race(player_t)`, `get_race(player_t)`
- Ship: `peek_ship(shipnum_t)`, `get_ship(shipnum_t)`, `num_ships()`
- Star: `peek_star(starnum_t)`, `get_star(starnum_t)`
- Planet: `peek_planet(starnum_t, planetnum_t)`, `get_planet(starnum_t, planetnum_t)`
- Sector: `peek_sector(planet_id, x, y)`, `get_sector(planet_id, x, y)`
- Commod: `peek_commod(id)`, `get_commod(id)`, `num_commods()`
- Block / Power / Universe / ServerState: same pattern.

## Composing Multiple Modifications

Each handle’s lifetime drives its own save. Hold all handles you intend to modify together; they each flush when their scope ends:

```cpp
auto race_handle   = g.entity_manager.get_race(g.player);
auto planet_handle = g.entity_manager.get_planet(snum, pnum);
auto& race   = *race_handle;
auto& planet = *planet_handle;

race.governor[g.governor].money -= cost;
planet.popn                     += migrants;
// Both auto-save on scope exit
```

For bulk turn processing call `entity_manager.flush_all()` and `clear_cache()` at the appropriate boundaries (see `do_turn`).

## Anti-Patterns

- ❌ Calling repositories or `getstar()`/`putstar()` etc. directly in new code.
- ❌ `if (!em.peek_star(snum)) ...` — `peek_star` throws, never returns null.
- ❌ Using `*em.get_xxx(id)` inline (see two-step rule above).
- ❌ Ignoring `g.race` and re-fetching the current player’s race for read-only access.
- ❌ Catching `EntityNotFoundError` for IDs that came from game state — that catches a programming error.

## Checklist

- [ ] `peek_*` for reads, `get_*` for writes
- [ ] Named local for every `get_*()` result before dereferencing
- [ ] No null checks on `peek_star/peek_planet/peek_sectormap/peek_race/peek_ship`
- [ ] try/catch only around user-input IDs
- [ ] `g.race->...` for current-player read access
- [ ] Tests set `g.race = em.peek_race(g.player);` after constructing `GameObj`
