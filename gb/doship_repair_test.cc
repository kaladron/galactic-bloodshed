// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

int main() {
  // Initialize in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  std::println("=== Testing do_repair via doship() ===\n");

  // Create ServerState with 1 segment for REPAIR_RATE division
  {
    ServerState state{};
    state.id = 1;
    state.segments = 1;

    JsonStore store(db);
    ServerStateRepository state_repo(store);
    state_repo.save(state);
  }

  // Create Race (owner)
  {
    Race race{};
    race.Playernum = 1;
    race.name = "TestRace";
    race.Guest = false;
    race.God = false;

    JsonStore store(db);
    RaceRepository races(store);
    races.save(race);
  }

  // Test 1: Basic repair scenario - ship with damage and crew
  {
    std::println("Test 1: Ship with crew and damage, sufficient resources");

    Ship ship{};
    ship.number() = 1;
    ship.owner() = player_t{1};
    ship.governor() = 0;
    ship.name() = "Scout1";
    ship.type() = ShipType::STYPE_SHUTTLE;  // Has crew, no ABIL_REPAIR
    ship.alive() = 1;
    ship.active() = 1;
    ship.damage() = 50;
    ship.popn() = 20;
    ship.max_crew() = 20;
    ship.resource() = 5000;
    ship.build_cost() = 100;
    ship.whatorbits() = ScopeLevel::LEVEL_UNIV;
    ship.docked() = 0;

    {
      JsonStore store(db);
      ShipRepository ships(store);
      ships.save(ship);
    }

    int damage_before = ship.damage();
    int resources_before = ship.resource();

    TurnStats stats;
    doship(ship, true, em, stats);

    {
      JsonStore store(db);
      ShipRepository ships(store);
      ships.save(ship);
    }

    em.clear_cache();
    const auto* saved_ship = em.peek_ship(ship.number());
    assert(saved_ship);

    std::println("  Damage:    {} -> {}", damage_before, saved_ship->damage());
    std::println("  Resources: {} -> {}", resources_before,
                 saved_ship->resource());

    // maxrep = REPAIR_RATE(25) / 1 segment * (20/20 crew) = 25.0
    // cost = (int)(0.005 * 25 * shipcost) = 12
    // drep = (int)25 = 25 -> damage: 50 - 25 = 25
    assert(saved_ship->damage() == 25);
    assert(saved_ship->resource() == 4988);
    std::println("  ✓ Test passed\n");
  }

  // Test 2: Ship with low crew (less efficient repair)
  {
    std::println("Test 2: Ship with minimal crew and damage");

    Ship ship{};
    ship.number() = 2;
    ship.owner() = player_t{1};
    ship.governor() = 0;
    ship.name() = "Scout2";
    ship.type() = ShipType::STYPE_SHUTTLE;
    ship.alive() = 1;
    ship.active() = 1;
    ship.damage() = 75;
    ship.popn() = 1;  // Minimal crew
    ship.max_crew() = 20;
    ship.resource() = 5000;
    ship.build_cost() = 100;
    ship.whatorbits() = ScopeLevel::LEVEL_UNIV;
    ship.docked() = 0;

    {
      JsonStore store(db);
      ShipRepository ships(store);
      ships.save(ship);
    }

    int damage_before = ship.damage();
    int resources_before = ship.resource();

    TurnStats stats;
    doship(ship, true, em, stats);

    {
      JsonStore store(db);
      ShipRepository ships(store);
      ships.save(ship);
    }

    em.clear_cache();
    const auto* saved_ship = em.peek_ship(ship.number());
    assert(saved_ship);

    std::println("  Damage:    {} -> {}", damage_before, saved_ship->damage());
    std::println("  Resources: {} -> {}", resources_before,
                 saved_ship->resource());

    // maxrep = REPAIR_RATE(25) / 1 segment * (1/20 crew) = 1.25
    // cost = (int)(0.005 * 1.25 * shipcost) = 0 (rounds down)
    // drep = (int)1.25 = 1 -> damage: 75 - 1 = 74
    // Resources unchanged because cost rounded to zero
    assert(saved_ship->damage() == 74);
    assert(saved_ship->resource() == 5000);
    std::println("  ✓ Test passed\n");
  }

  // Test 3: Ship without damage (repair should not be triggered)
  {
    std::println("Test 3: Ship with no damage");

    Ship ship{};
    ship.number() = 3;
    ship.owner() = player_t{1};
    ship.governor() = 0;
    ship.name() = "Scout3";
    ship.type() = ShipType::STYPE_SHUTTLE;
    ship.alive() = 1;
    ship.active() = 1;
    ship.damage() = 0;  // No damage
    ship.popn() = 20;
    ship.max_crew() = 20;
    ship.resource() = 5000;
    ship.build_cost() = 100;
    ship.whatorbits() = ScopeLevel::LEVEL_UNIV;
    ship.docked() = 0;

    {
      JsonStore store(db);
      ShipRepository ships(store);
      ships.save(ship);
    }

    int resources_before = ship.resource();

    TurnStats stats;
    doship(ship, true, em, stats);

    {
      JsonStore store(db);
      ShipRepository ships(store);
      ships.save(ship);
    }

    em.clear_cache();
    const auto* saved_ship = em.peek_ship(ship.number());
    assert(saved_ship);

    std::println("  Damage:    0 -> {}", saved_ship->damage());
    std::println("  Resources: {} -> {}", resources_before,
                 saved_ship->resource());

    // No damage means do_repair is never called (guarded by ship.damage() check
    // in doship)
    assert(saved_ship->damage() == 0);
    assert(saved_ship->resource() == 5000);
    std::println("  ✓ Correctly no repair (no damage)\n");
  }

  // Test 4: Factory type (has ABIL_REPAIR)
  {
    std::println("Test 4: Factory ship (has ABIL_REPAIR ability)");

    Ship ship{};
    ship.number() = 4;
    ship.owner() = player_t{1};
    ship.governor() = 0;
    ship.name() = "Factory1";
    ship.type() = ShipType::OTYPE_FACTORY;  // Has ABIL_REPAIR
    ship.alive() = 1;
    ship.active() = 1;
    ship.on() = 1;  // Factory is on
    ship.damage() = 60;
    ship.popn() = 0;  // Factories have no crew
    ship.max_crew() = 0;
    ship.resource() = 5000;
    ship.build_cost() = 1000;
    ship.whatorbits() = ScopeLevel::LEVEL_UNIV;
    ship.docked() = 0;

    {
      JsonStore store(db);
      ShipRepository ships(store);
      ships.save(ship);
    }

    int damage_before = ship.damage();
    int resources_before = ship.resource();

    TurnStats stats;
    doship(ship, true, em, stats);

    {
      JsonStore store(db);
      ShipRepository ships(store);
      ships.save(ship);
    }

    em.clear_cache();
    const auto* saved_ship = em.peek_ship(ship.number());
    assert(saved_ship);

    std::println("  Damage:    {} -> {}", damage_before, saved_ship->damage());
    std::println("  Resources: {} -> {}", resources_before,
                 saved_ship->resource());

    // ABIL_REPAIR path: cost=0, maxrep = REPAIR_RATE(25) / 1 = 25.0
    // drep = (int)25 = 25 -> damage: 60 - 25 = 35
    // No resources consumed (free repair for factories)
    assert(saved_ship->damage() == 35);
    assert(saved_ship->resource() == 5000);
    std::println("  ✓ Test passed\n");
  }

  std::println("=== All do_repair tests completed ===");
  std::println(
      "Review output above to verify repair logic is functioning correctly.");
  return 0;
}
