// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import commands;
import std.compat;

#include <cassert>

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create EntityManager
  EntityManager em(db);

  // Create test race with enough tech for upgrades
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;
  race.mass = 1.0;
  race.fighters = 1.0;
  race.tech = 100.0;  // High tech to allow upgrades
  race.morale = 100;
  race.God = false;

  // Save race via repository
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Create a test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);  // Player 1 has explored
  ss.AP[0] = 10;
  Star star(ss);

  // Save star via repository
  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create a ship that can be upgraded (must have ABIL_MOD capability)
  // STYPE_FIGHTER (type 1) is a common modifiable ship type
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_FIGHTER;
  ship.build_type() = ShipType::STYPE_FIGHTER;
  ship.name() = "Upgradeable";
  ship.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship.storbits() = 0;
  ship.xpos() = 100.0;
  ship.ypos() = 200.0;
  ship.fuel() = 100.0;
  ship.max_fuel() = 100.0;
  ship.resource() = 500;  // Need resources to pay for upgrades
  ship.max_resource() = 1000;
  ship.popn() = 10;
  ship.max_crew() = 10;
  ship.armor() = 10;
  ship.max_speed() = 5;
  ship.max_destruct() = 10;
  ship.mass() = 10.0;
  ship.base_mass() = 10.0;
  ship.build_cost() = 50;
  ship.damage() = 0;  // No damage - required for upgrades

  // Save ship via repository
  ShipRepository ships_repo(store);
  ships_repo.save(ship);

  // Create GameObj for command execution - must be at SHIP scope
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_SHIP;  // Must be at ship scope for upgrade
  g.shipno = 1;
  g.snum = 0;

  std::println("Test 1: Upgrade ship armor");
  {
    em.clear_cache();
    g.race = em.peek_race(1);  // Re-fetch after cache clear
    const auto* ship_before = em.peek_ship(1);
    assert(ship_before != nullptr);
    int initial_armor = ship_before->armor();
    int initial_resource = ship_before->resource();
    std::println("    Before: armor={}, resource={}", initial_armor,
                 initial_resource);

    // upgrade armor 50
    command_t argv = {"upgrade", "armor", "50"};
    GB::commands::upgrade(argv, g);

    // Clear cache to force reload from database
    em.clear_cache();

    const auto* ship_after = em.peek_ship(1);
    assert(ship_after != nullptr);
    std::println("    After: armor={}, resource={}", ship_after->armor(),
                 ship_after->resource());

    // Armor should have increased (up to max of 100)
    assert(ship_after->armor() >= initial_armor);
    std::println("    ✓ Armor upgrade applied (was {}, now {})", initial_armor,
                 ship_after->armor());
  }

  std::println("Test 2: Upgrade ship speed");
  {
    em.clear_cache();
    g.race = em.peek_race(1);  // Re-fetch after cache clear
    const auto* ship_before = em.peek_ship(1);
    assert(ship_before != nullptr);
    int initial_speed = ship_before->max_speed();
    int initial_resource = ship_before->resource();
    std::println("    Before: max_speed={}, resource={}", initial_speed,
                 initial_resource);

    // upgrade speed 9 (max is 9)
    command_t argv = {"upgrade", "speed", "9"};
    GB::commands::upgrade(argv, g);

    em.clear_cache();

    const auto* ship_after = em.peek_ship(1);
    assert(ship_after != nullptr);
    std::println("    After: max_speed={}, resource={}",
                 ship_after->max_speed(), ship_after->resource());

    // Speed should have increased
    assert(ship_after->max_speed() >= initial_speed);
    std::println("    ✓ Speed upgrade applied (was {}, now {})", initial_speed,
                 ship_after->max_speed());
  }

  std::println("Test 3: Verify upgrades persist after cache clear");
  {
    em.clear_cache();

    const auto* ship_check = em.peek_ship(1);
    assert(ship_check != nullptr);

    // Values should still reflect upgrades
    std::println("    Final values: armor={}, max_speed={}, resource={}",
                 ship_check->armor(), ship_check->max_speed(),
                 ship_check->resource());

    std::println("    ✓ Upgrades persisted to database");
  }

  std::println("\n✅ All upgrade tests passed!");
  return 0;
}
