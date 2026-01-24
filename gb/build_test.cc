// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  // Initialize database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Create a test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 50.0;

  // Create a test planet with sectors
  Planet planet{};
  planet.Maxx() = 10;
  planet.Maxy() = 10;

  // Create a normal sector owned by the race with population
  Sector good_sector{};
  good_sector.set_owner(1);
  good_sector.set_popn_exact(100);
  good_sector.set_condition(SectorType::SEC_LAND);

  // Test 1: Success case - can build on owned sector with population
  {
    auto result = can_build_on_sector(em, ShipType::OTYPE_PROBE, race, planet,
                                      good_sector, {0, 0});
    assert(result.has_value());
    std::println("Test 1 passed: Can build on valid sector");
  }

  // Test 2: Fail - no population
  {
    Sector no_pop_sector{};
    no_pop_sector.set_owner(1);
    no_pop_sector.clear_popn();
    no_pop_sector.set_condition(SectorType::SEC_LAND);
    auto result = can_build_on_sector(em, ShipType::OTYPE_PROBE, race, planet,
                                      no_pop_sector, {0, 0});
    assert(!result.has_value());
    assert(result.error() == "You have no more civs in the sector!\n");
    std::println("Test 2 passed: Rejects sector with no population");
  }

  // Test 3: Fail - wasted sector
  {
    Sector wasted_sector{};
    wasted_sector.set_owner(1);
    wasted_sector.set_popn_exact(100);
    wasted_sector.set_condition(SectorType::SEC_WASTED);
    auto result = can_build_on_sector(em, ShipType::OTYPE_PROBE, race, planet,
                                      wasted_sector, {0, 0});
    assert(!result.has_value());
    assert(result.error() == "You can't build on wasted sectors.\n");
    std::println("Test 3 passed: Rejects wasted sector");
  }

  // Test 4: Fail - sector not owned by race
  {
    Sector alien_sector{};
    alien_sector.set_owner(2);  // Different player
    alien_sector.set_popn_exact(100);
    alien_sector.set_condition(SectorType::SEC_LAND);
    auto result = can_build_on_sector(em, ShipType::OTYPE_PROBE, race, planet,
                                      alien_sector, {0, 0});
    assert(!result.has_value());
    assert(result.error() == "You don't own that sector.\n");
    std::println("Test 4 passed: Rejects sector owned by another player");
  }

  // Test 5: Success - God can build on alien sector
  {
    Race god_race = race;
    god_race.God = true;
    Sector alien_sector{};
    alien_sector.set_owner(2);
    alien_sector.set_popn_exact(100);
    alien_sector.set_condition(SectorType::SEC_LAND);
    auto result = can_build_on_sector(em, ShipType::OTYPE_PROBE, god_race,
                                      planet, alien_sector, {0, 0});
    assert(result.has_value());
    std::println("Test 5 passed: God can build on alien sector");
  }

  // Test 6: Fail - ship type cannot be built on planet (non-God)
  {
    // Find a ship type that cannot be built on planets (ABIL_BUILD bit 0 not
    // set) Using STYPE_HABITAT which typically can't be built on planets
    if (!(Shipdata[ShipType::STYPE_HABITAT][ABIL_BUILD] & 1)) {
      auto result = can_build_on_sector(em, ShipType::STYPE_HABITAT, race,
                                        planet, good_sector, {0, 0});
      assert(!result.has_value());
      assert(result.error().find("cannot be built on a planet") !=
             std::string::npos);
      std::println("Test 6 passed: Rejects ship type that can't be built on "
                   "planets");
    } else {
      std::println("Test 6 skipped: HABITAT can be built on planets in this "
                   "configuration");
    }
  }

  // Test 7: Success - quarry at new location
  {
    auto result = can_build_on_sector(em, ShipType::OTYPE_QUARRY, race, planet,
                                      good_sector, {5, 5});
    assert(result.has_value());
    std::println("Test 7 passed: Can build quarry at empty location");
  }

  // Test 8: Fail - quarry already exists at location (3rd ship in list)
  {
    // Create first ship - a probe at different location
    Ship probe1{};
    probe1.number() = 100;
    probe1.type() = ShipType::OTYPE_PROBE;
    probe1.owner() = 1;
    probe1.alive() = true;
    probe1.land_x() = 1;
    probe1.land_y() = 1;
    probe1.nextship() = 101;  // Links to second ship

    // Create second ship - another probe at different location
    Ship probe2{};
    probe2.number() = 101;
    probe2.type() = ShipType::OTYPE_PROBE;
    probe2.owner() = 1;
    probe2.alive() = true;
    probe2.land_x() = 2;
    probe2.land_y() = 2;
    probe2.nextship() = 102;  // Links to third ship (the quarry)

    // Create third ship - a quarry at coordinates (3, 3)
    Ship quarry{};
    quarry.number() = 102;
    quarry.type() = ShipType::OTYPE_QUARRY;
    quarry.owner() = 1;
    quarry.alive() = true;
    quarry.land_x() = 3;
    quarry.land_y() = 3;
    quarry.nextship() = 0;  // End of list

    // Add all ships to planet's ship list (starts with ship 100)
    planet.ships() = 100;
    JsonStore store(db);
    ShipRepository ships(store);
    ships.save(probe1);
    ships.save(probe2);
    ships.save(quarry);

    // Try to build another quarry at same location as the 3rd ship
    auto result = can_build_on_sector(em, ShipType::OTYPE_QUARRY, race, planet,
                                      good_sector, {3, 3});
    assert(!result.has_value());
    assert(result.error() == "There already is a quarry here.\n");
    std::println(
        "Test 8 passed: Rejects duplicate quarry at same location (3rd ship "
        "in list)");
  }

  // Test 9: Success - quarry at different location than existing
  {
    // Quarry exists at (3,3), try building at (4,4)
    auto result = can_build_on_sector(em, ShipType::OTYPE_QUARRY, race, planet,
                                      good_sector, {4, 4});
    assert(result.has_value());
    std::println(
        "Test 9 passed: Can build quarry at different location than existing");
  }

  // Test 10: Success - dead quarry at location doesn't block
  {
    // Create a dead quarry at (7, 7)
    Ship dead_quarry{};
    dead_quarry.number() = 2;
    dead_quarry.type() = ShipType::OTYPE_QUARRY;
    dead_quarry.owner() = 1;
    dead_quarry.alive() = false;  // Dead
    dead_quarry.land_x() = 7;
    dead_quarry.land_y() = 7;

    JsonStore store(db);
    ShipRepository ships(store);
    ships.save(dead_quarry);

    // Should be able to build at (7,7) since existing quarry is dead
    auto result = can_build_on_sector(em, ShipType::OTYPE_QUARRY, race, planet,
                                      good_sector, {7, 7});
    assert(result.has_value());
    std::println("Test 10 passed: Dead quarry doesn't block new construction");
  }

  std::println("\nAll can_build_on_sector tests passed!");
  return 0;
}
