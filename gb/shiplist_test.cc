// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create EntityManager for accessing entities
  EntityManager em(db);

  // Create JsonStore for repository operations
  JsonStore store(db);

  // Create a test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.governor[0].money = 1000;

  RaceRepository races(store);
  races.save(race);

  // Create test ships
  Ship ship1{};
  ship1.number = 1;
  ship1.owner = 1;
  ship1.alive = true;
  ship1.storbits = 0;
  ship1.pnumorbits = 0;
  ship1.type = ShipType::OTYPE_FACTORY;
  ship1.nextship = 2;  // Linked list

  Ship ship2{};
  ship2.number = 2;
  ship2.owner = 1;
  ship2.alive = true;
  ship2.storbits = 0;
  ship2.pnumorbits = 0;
  ship2.type = ShipType::OTYPE_PROBE;
  ship2.nextship = 3;

  Ship ship3{};
  ship3.number = 3;
  ship3.owner = 1;
  ship3.alive = true;
  ship3.storbits = 0;
  ship3.pnumorbits = 0;
  ship3.type = ShipType::STYPE_CARGO;
  ship3.nextship = 0;  // End of list

  ShipRepository ships_repo(store);
  ships_repo.save(ship1);
  ships_repo.save(ship2);
  ships_repo.save(ship3);

  // Test 1: Nested iteration (follows nextship linked list)
  {
    ShipList list(em, 1);  // Start at ship 1, nested iteration
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      assert(ship.alive);
      assert(ship.owner == 1);
    }
    assert(count == 3);
    std::println("✓ Test 1 passed: Nested iteration found {} ships", count);
  }

  // Create GameObj for scope-based tests
  GameObj g(em);
  g.player = 1;
  g.snum = 0;
  g.pnum = 0;
  g.race = em.peek_race(1);

  // Test 3: Scope iteration at universe level
  {
    ShipList list(em, g, ShipList::IterationType::Scope);
    int count = 0;
    for (auto handle : list) {
      count++;
      Ship& ship = *handle;
      assert(ship.alive);
    }
    assert(count == 3);
    std::println("✓ Test 2 passed: Scope iteration (UNIV) found {} ships",
                 count);
  }

  // Test 3: Modify ship via handle
  {
    ShipList list(em, 1, ShipList::IterationType::Nested);
    auto it = list.begin();
    ShipHandle handle = *it;
    Ship& ship = *handle;

    ship.fuel += 100.0;
    // Handle should auto-save on destruction
  }

  // Verify modification persisted
  {
    const auto* ship = em.peek_ship(1);
    assert(ship->fuel >= 100.0);
    std::println("✓ Test 3 passed: Ship modification persisted via RAII");
  }

  std::println("\nAll ShipList tests passed!");
  return 0;
}
