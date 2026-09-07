---
name: database-test-pattern
description: 'Write unit tests that exercise EntityManager and repositories against an in-memory SQLite database. Covers TestContext, with_standard_universe fixture, TestShipBuilder, low-level in-memory DB setup, schema init order, cache-clear verification, GameObj construction in tests, and CMake/CTest wiring.'
user-invocable: false
---

# Database Test Pattern

Tests in this codebase run against a real in-memory SQLite instance. The same EntityManager, repositories, and DAL code paths the server uses are exercised — so persistence is actually tested, not just in-memory mutation.

## Preferred Harness: `TestContext` & Standard Universe Fixture

For almost all unit, command, and turn simulation tests, use `TestContext` from the `test` module (`import test;`). It eliminates manual database connection, schema initialization, and repository boilerplate:

```cpp
// SPDX-License-Identifier: Apache-2.0

import dallib;
import gb.entities;
import gb.services;
import test;
import std;

#include <cassert>

void test_something() {
  TestContext ctx;
  ctx.with_standard_universe();  // Provisions Sol (Star 0), Earth (Planet 0), Vega (Star 1),
                                 // Vega Prime (Planet 0), Federation (Player 1), Klingons (Player 2),
                                 // 100 AP each, and alliance blocks

  // Create test ships using the fluent builder populated with canonical ShipTemplate defaults
  shipnum_t ship_id = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                          .owned_by(1, 0)
                          .in_star_orbit(0)
                          .with_fuel(100.0)
                          .build();

  // Exercise domain operations via monadic mutation
  ctx.em.mutate_ship(ship_id, [&](Ship& ship) {
    ship.damage() = 25;
  });

  // Verify persistence via EntityManager peek
  const auto* ship = ctx.em.peek_ship(ship_id);
  test::expect_eq(ship->damage(), 25);

  // Optional: verify all cross-entity referential invariants
  ctx.verify_universe_invariants();
}
```

### What `TestContext` Provides:
- **Automatic DB & Schema**: Creates `Database db(":memory:")` and runs `initialize_schema(db)` automatically.
- **Default Universe**: `ctx.with_standard_universe()` provisions a canonical 2-player solar system with Sol and Vega systems, populated Earth and Vega Prime, 100 AP, and alliance blocks.
- **Populated Planets**: `ctx.with_populated_planet(snum, pnum, owner, popn)` colonizes and seeds population on a planet sector while keeping planet and sectormap populations synchronized.
- **`TestShipBuilder`**: Fluent builder with canonical `ShipTemplate` defaults (armor, crew capacity, cargo, weapons, speed), eliminating magic numbers in test setups.
- **`ctx.setup_game_obj(g, player, gov)`**: Sets player, governor, and automatically assigns `g.race = ctx.em.peek_race(player)`.
- **Command Dispatch & AP Assertions**: `ctx.assert_dispatch_success(...)` and `ctx.assert_dispatch_rejected(...)` automatically verify action point deductions and rollbacks.

---

## Low-Level Setup Order (When Not Using `TestContext`)

When testing low-level DAL components, custom migration scripts, or raw database connection behavior without the full game service stack, construct the connection and schema manually in strict order:

```cpp
// SPDX-License-Identifier: Apache-2.0

import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import std;

#include <cassert>

int main() {
  Database db(":memory:");   // 1. Create connection FIRST
  initialize_schema(db);     // 2. Create tables on that connection
  EntityManager em(db);      // 3. Build services on top
  JsonStore store(db);       // 4. Optional, for direct repo access

  // ... test body ...

  std::println("Test passed!");
  return 0;
}
```

If `initialize_schema(db)` is skipped or runs against the wrong connection, every later access segfaults or fails with "no such table". This is the most common cause of low-level test failures.

## Universe Setup via Repositories

When manually populating entities outside `TestContext::with_standard_universe()`, create entities **through the repositories**, not by writing into internal caches:

```cpp
JsonStore store(db);
RaceRepository races(store);

Race race{};
race.Playernum = player_t{1};
race.name      = "TestRace";
race.Guest     = false;
race.governor[0].active = true;
race.governor[0].money  = 1000;
races.save(race);
```

Then exercise the code under test through `EntityManager`.

## Cache-Clear-and-Verify Pattern

To prove a change actually persisted (vs only sitting in the cache):

```cpp
// 1. Initial state — clear cache to force a DB read
em.clear_cache();
{
  const auto* r = em.peek_race(player_t{1});
  assert(r);
  assert(r->governor[0].money == 1000);
}

// 2. Mutate via the production code path
em.mutate_race(player_t{1}, [](Race& race) {
  race.governor[0].money += 500;
});  // auto-save fires on lambda exit

// 3. Clear cache again and re-read from disk
em.clear_cache();
const auto* after = em.peek_race(player_t{1});
assert(after->governor[0].money == 1500);
```

Do this for any test claiming "persists changes". Pure in-memory assertions only prove the cache mutated.

## Testing a Command Directly

Commands take `GameObj& g`. Using `TestContext`:

```cpp
TestContext ctx;
ctx.with_standard_universe();

auto& registry = get_test_session_registry();
GameObj g(ctx.em, registry);
ctx.setup_game_obj(g, 1, 0);  // Sets player 1, gov 0, and g.race = peek_race(1)
g.set_level(ScopeLevel::LEVEL_PLAN);
g.set_snum(0);
g.set_pnum(0);

// Dispatch through command descriptor or TestContext
bool ok = ctx.dispatch(g, {"commandname", "arg1"});
test::expect_true(ok);

// Assert observable side effects via EntityManager
const auto* planet = ctx.em.peek_planet(0, 0);
test::expect_eq(planet->popn(), expected);
```

Forgetting to set `g.race` is a common bug when setting up `GameObj` manually — `ctx.setup_game_obj(g)` guarantees `g.race` is pre-populated.

## Multi-Test Files

Group related cases as separate functions called from `main()`:

```cpp
void test_persistence() { /* ... */ std::println("✓ persistence"); }
void test_validation()  { /* ... */ std::println("✓ validation");  }

int main() {
  test_persistence();
  test_validation();
  std::println("\n✅ all passed");
  return 0;
}
```

Each test function should construct its own `TestContext` so cases stay isolated.

## CMake / CTest Wiring

Add to `gb/CMakeLists.txt`:

```cmake
add_executable(commandname_test commands/commandname_test.cc)
target_link_libraries(commandname_test
  PRIVATE dallib gblib commands test SQLite::SQLite3 glaze::glaze)
add_test(NAME commandname_test COMMAND commandname_test)
```

Run from the workspace root:

```bash
ninja -C build commandname_test
(cd build && ctest -R commandname_test --verbose)
```

## Anti-Patterns

- ❌ Re-inventing manual `Database db(":memory:"); initialize_schema(db);` boilerplate when `TestContext` / `with_standard_universe()` can be used.
- ❌ Hardcoding raw ship metrics (size, armor, cargo) instead of using `TestShipBuilder` with canonical `ShipTemplate` defaults.
- ❌ Calling `initialize_schema(db)` before constructing `Database`.
- ❌ Pointing `Database` at a real on-disk path in a test.
- ❌ Skipping `em.clear_cache()` when claiming a persistence guarantee.
- ❌ Building entities by directly manipulating `EntityManager` internals instead of going through repositories or `TestContext` builders.
- ❌ Forgetting `g.race = em.peek_race(g.player);` after constructing `GameObj` (use `ctx.setup_game_obj(g)`).
- ❌ Sharing one `Database` or `TestContext` across unrelated test cases.

## Checklist

- [ ] Prefer `TestContext ctx; ctx.with_standard_universe();` for command and domain tests
- [ ] Use `TestShipBuilder` for test ship construction to preserve template baselines
- [ ] For raw DB tests: `Database db(":memory:")` first, then `initialize_schema(db)`
- [ ] `EntityManager em(db)` constructed after schema init
- [ ] Test entities created via repositories or fluent builders, not cache
- [ ] `em.clear_cache()` between mutate and re-read for persistence assertions
- [ ] `ctx.setup_game_obj(g)` used whenever a `GameObj` is built
- [ ] Test executable wired in `gb/CMakeLists.txt` with `add_test(...)`
- [ ] `ctest -R <name>` passes from a clean build

