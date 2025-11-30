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

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;

  // Save race via repository
  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Create a test star using star_struct, then wrap in Star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);  // Player 1 has explored this star (bit 1)
  Star star(ss);

  // Save star via repository
  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create a test ship orbiting the star
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_FIGHTER;
  ship.name() = "TestFighter";
  ship.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship.storbits() = 0;  // Orbiting star 0
  ship.xpos() = 100.0;
  ship.ypos() = 200.0;

  // Save ship via repository
  ShipRepository ships_repo(store);
  ships_repo.save(ship);

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);  // Set race pointer like production
  g.level = ScopeLevel::LEVEL_STAR;
  g.snum = 0;  // At star 0

  std::println("Test 1: Orbit command displays ship at star");
  {
    // orbit command should display ships in current scope
    // This is a read-only display command
    command_t argv = {"orbit"};
    GB::commands::orbit(argv, g);

    // Verify ship is still the same (not modified)
    const auto* saved_ship = em.peek_ship(1);
    assert(saved_ship != nullptr);
    assert(saved_ship->owner() == 1);
    assert(saved_ship->whatorbits() == ScopeLevel::LEVEL_STAR);
    assert(saved_ship->storbits() == 0);
    std::println("    ✓ Orbit display works correctly");
  }

  std::println("Test 2: Orbit at universe level");
  {
    // Change scope to universe level
    g.level = ScopeLevel::LEVEL_UNIV;

    command_t argv = {"orbit"};
    GB::commands::orbit(argv, g);

    // Verify ship is still unchanged
    const auto* saved_ship = em.peek_ship(1);
    assert(saved_ship != nullptr);
    assert(saved_ship->owner() == 1);
    std::println("    ✓ Universe-level orbit works correctly");
  }

  std::println("\n✅ All orbit tests passed!");
  return 0;
}
