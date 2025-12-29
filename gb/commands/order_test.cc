// SPDX-License-Identifier: Apache-2.0

import dallib;
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

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;

  // Save race via repository
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Create a test ship with default orders - use battleship which can bombard
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_BATTLE;  // Battleship can bombard
  ship.name() = "TestShip";
  ship.speed() = 5;
  ship.popn() = 100;  // Crew

  // Save ship via repository
  ShipRepository ships_repo(store);
  ships_repo.save(ship);

  // Create GameObj for command execution
  GameObj g(em);
  g.set_player(1);
  g.set_governor(0);
  g.race = em.peek_race(1);  // Set race pointer like production
  g.set_level(ScopeLevel::LEVEL_UNIV);

  std::println("Test 1: Set ship defense order");
  {
    // Clear cache to ensure we get fresh data
    em.clear_cache();

    command_t argv = {"order", "#1", "defense", "on"};
    GB::commands::order(argv, g);

    // Clear cache again to force reload from database
    em.clear_cache();

    // Verify defense order was set
    const auto* saved_ship = em.peek_ship(1);
    assert(saved_ship != nullptr);
    std::println("    Ship found: number={}, protect.planet={}",
                 saved_ship->number(), saved_ship->protect().planet);
    assert(saved_ship->protect().planet == 1);
    std::println("    ✓ Defense order set: protect.planet={}",
                 saved_ship->protect().planet);
  }

  std::println("\nTest 2: Turn defense order off");
  {
    // Clear cache to ensure we get fresh data
    em.clear_cache();

    command_t argv = {"order", "#1", "defense", "off"};
    GB::commands::order(argv, g);

    // Clear cache again to force reload from database
    em.clear_cache();

    // Verify defense was turned off
    const auto* saved_ship = em.peek_ship(1);
    assert(saved_ship != nullptr);
    assert(saved_ship->protect().planet == 0);
    std::println("    ✓ Defense order turned off: protect.planet={}",
                 saved_ship->protect().planet);
  }

  std::println("\nTest 3: Display all orders (no modifications)");
  {
    // This should just display orders without modifications
    command_t argv = {"order"};
    GB::commands::order(argv, g);

    // Verify ship state unchanged
    const auto* saved_ship = em.peek_ship(1);
    assert(saved_ship != nullptr);
    assert(saved_ship->protect().planet == 0);  // Still off from previous test
    std::println("    ✓ Display orders works without modification");
  }

  std::println("\n✅ All order tests passed!");
  return 0;
}
