---
name: database-test-pattern
description: 'Write unit tests that exercise EntityManager and repositories against an in-memory SQLite database. Use when adding a *_test.cc file under gb/, verifying database persistence, testing a command end-to-end, or debugging segfaults caused by missing schema initialization. Covers the in-memory DB setup, schema init order, cache-clear verification, GameObj construction in tests, and CMake/CTest wiring.'
user-invocable: false
---

# Database Test Pattern

Tests in this codebase run against a real in-memory SQLite instance. The same EntityManager, repositories, and DAL code paths the server uses are exercised — so persistence is actually tested, not just in-memory mutation.

## Mandatory Setup Order

```cpp
// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
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

If `initialize_schema(db)` is skipped or runs against the wrong connection, every later access segfaults or fails with "no such table". This is the most common cause of new-test failures.

## Universe Setup via Repositories

Create the entities your test needs **through the repositories**, not by writing into the cache. This mirrors how `enrol`/`makeuniv` populate the database in production:

```cpp
JsonStore store(db);
RaceRepository races(store);

Race race{};
race.Playernum = player_t{1};
race.name      = "TestRace";
race.Guest     = false;
race.governor[0].money = 1000;
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
{
  auto handle = em.get_race(player_t{1});
  auto& race = *handle;
  race.governor[0].money += 500;
}  // auto-save fires here

// 3. Clear cache again and re-read from disk
em.clear_cache();
const auto* after = em.peek_race(player_t{1});
assert(after->governor[0].money == 1500);
```

Do this for any test claiming "persists changes". Pure in-memory assertions only prove the cache mutated.

## Testing a Command Directly

Commands take `GameObj& g`. Construct one in the test and mirror what `process_command` does:

```cpp
GameObj g(em);
g.player    = player_t{1};
g.governor  = governor_t{0};
g.race      = em.peek_race(g.player);   // production sets this; tests must too
g.level     = ScopeLevel::LEVEL_PLAN;
g.snum      = 0;
g.pnum      = 0;

GB::commands::commandname({"commandname", "arg1"}, g);

// Assert observable side effects
em.clear_cache();
const auto* planet = em.peek_planet(0, 0);
assert(planet->popn == expected);
```

Forgetting to set `g.race` is the second most common test bug — production guarantees it is non-null.

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

Each test function should set up its own `Database`/`EntityManager` so cases stay independent.

## CMake / CTest Wiring

Add to `gb/CMakeLists.txt`:

```cmake
add_executable(commandname_test commands/commandname_test.cc)
target_link_libraries(commandname_test
  PRIVATE dallib gblib commands SQLite::SQLite3 glaze::glaze)
add_test(NAME commandname_test COMMAND commandname_test)
```

Run from the workspace root:

```bash
ninja -C build commandname_test
(cd build && ctest -R commandname_test --verbose)
```

## Anti-Patterns

- ❌ Calling `initialize_schema(db)` before constructing `Database`.
- ❌ Pointing `Database` at a real on-disk path in a test.
- ❌ Skipping `em.clear_cache()` when claiming a persistence guarantee.
- ❌ Building entities by directly manipulating `EntityManager` internals instead of going through repositories.
- ❌ Forgetting `g.race = em.peek_race(g.player);` after constructing `GameObj`.
- ❌ Sharing one `Database` across unrelated test cases.

## Checklist

- [ ] `Database db(":memory:")` first, then `initialize_schema(db)`
- [ ] `EntityManager em(db)` constructed after schema init
- [ ] Test entities created via repositories, not cache
- [ ] `em.clear_cache()` between mutate and re-read for persistence assertions
- [ ] `g.race = em.peek_race(g.player);` whenever a `GameObj` is built
- [ ] Test executable wired in `gb/CMakeLists.txt` with `add_test(...)`
- [ ] `ctest -R <name>` passes from a clean build
