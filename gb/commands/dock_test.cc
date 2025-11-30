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
  race.mass = 1.0;
  race.fighters = 1.0;
  race.tech = 10.0;
  race.morale = 100;

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
  ss.AP[1] = 10;              // Player 1 has APs
  Star star(ss);

  // Save star via repository
  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create first ship (the one doing the docking)
  Ship ship1{};
  ship1.number() = 1;
  ship1.owner() = 1;
  ship1.governor() = 0;
  ship1.alive() = true;
  ship1.active() = true;
  ship1.type() = ShipType::STYPE_FIGHTER;
  ship1.name() = "Docker";
  ship1.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship1.storbits() = 0;  // Orbiting star 0
  ship1.xpos() = 100.0;
  ship1.ypos() = 200.0;
  ship1.fuel() = 100.0;
  ship1.mass() = 10.0;
  ship1.docked() = 0;

  // Create second ship (target for docking) - must be close enough
  Ship ship2{};
  ship2.number() = 2;
  ship2.owner() = 1;
  ship2.governor() = 0;
  ship2.alive() = true;
  ship2.active() = true;
  ship2.type() = ShipType::STYPE_CARRIER;
  ship2.name() = "Target";
  ship2.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship2.storbits() = 0;  // Same star
  ship2.xpos() = 100.0;  // Very close to ship1
  ship2.ypos() = 200.0;
  ship2.fuel() = 100.0;
  ship2.mass() = 100.0;
  ship2.docked() = 0;

  // Save ships via repository
  ShipRepository ships_repo(store);
  ships_repo.save(ship1);
  ships_repo.save(ship2);

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);  // Set race pointer like production
  g.level = ScopeLevel::LEVEL_STAR;
  g.snum = 0;  // At star 0

  std::println("Test 1: Dock ship #1 with ship #2");
  {
    // Clear cache to ensure we get fresh data
    em.clear_cache();

    // dock #1 #2
    command_t argv = {"dock", "#1", "#2"};
    GB::commands::dock(argv, g);

    // Clear cache to force reload from database
    em.clear_cache();

    // Verify both ships are now docked
    const auto* saved_ship1 = em.peek_ship(1);
    const auto* saved_ship2 = em.peek_ship(2);
    assert(saved_ship1 != nullptr);
    assert(saved_ship2 != nullptr);

    std::println("    Ship 1: docked={}, whatdest={}, destshipno={}",
                 saved_ship1->docked(),
                 static_cast<int>(saved_ship1->whatdest()),
                 saved_ship1->destshipno());
    std::println("    Ship 2: docked={}, whatdest={}, destshipno={}",
                 saved_ship2->docked(),
                 static_cast<int>(saved_ship2->whatdest()),
                 saved_ship2->destshipno());

    assert(saved_ship1->docked() == 1);
    assert(saved_ship1->whatdest() == ScopeLevel::LEVEL_SHIP);
    assert(saved_ship1->destshipno() == 2);

    assert(saved_ship2->docked() == 1);
    assert(saved_ship2->whatdest() == ScopeLevel::LEVEL_SHIP);
    assert(saved_ship2->destshipno() == 1);

    std::println("    ✓ Both ships are now docked with each other");
  }

  std::println("Test 2: Verify docked ships persist after cache clear");
  {
    // Already cleared above, but let's verify again
    em.clear_cache();

    const auto* ship1_check = em.peek_ship(1);
    const auto* ship2_check = em.peek_ship(2);

    assert(ship1_check != nullptr);
    assert(ship2_check != nullptr);
    assert(ship1_check->docked() == 1);
    assert(ship2_check->docked() == 1);
    assert(ship1_check->destshipno() == 2);
    assert(ship2_check->destshipno() == 1);

    std::println("    ✓ Docking status persisted to database");
  }

  std::println("\n✅ All dock tests passed!");
  return 0;
}
