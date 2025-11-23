// SPDX-License-Identifier: Apache-2.0

/// \file autoreport_test.cc
/// \brief Test autoreport command database persistence

import dallib;
import gblib;
import commands;
import std;

#include <cassert>

void test_autoreport_database_persistence() {
  std::println("Test: autoreport command database persistence");

  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Setup: Create a star
  star_struct star{};
  star.star_id = 1;
  star.name = "Test Star";
  star.pnames.push_back("Test Planet");
  star.governor[0] = 0;  // Player 1 (index 0) governor 0

  JsonStore store(db);
  StarRepository stars(store);
  stars.save(star);

  // Setup: Create a planet with autoreport initially OFF
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(0).numsectsowned = 10;  // Player 1 owns sectors
  planet.info(0).autorep = 0;         // Initially OFF

  PlanetRepository planets(store);
  planets.save(planet);

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.level = ScopeLevel::LEVEL_PLAN;
  g.snum = 1;
  g.pnum = 0;

  // TEST 1: Toggle autoreport ON
  std::println("  Testing: Toggle autoreport ON");
  {
    command_t cmd = {"autoreport"};
    GB::commands::autoreport(cmd, g);

    // Verify output message
    std::string out_str = g.out.str();
    assert(out_str.find("has been set") != std::string::npos);
    std::println("    ✓ Output message correct");
    g.out.str("");  // Clear output for next test

    // Verify database: autorep should be TELEG_MAX_AUTO (63)
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).autorep == TELEG_MAX_AUTO);
    std::println("    ✓ Database: autorep = {} (ON)", saved->info(0).autorep);
  }

  // TEST 2: Toggle autoreport OFF
  std::println("  Testing: Toggle autoreport OFF");
  {
    command_t cmd = {"autoreport"};
    GB::commands::autoreport(cmd, g);

    // Verify output message
    std::string out_str = g.out.str();
    assert(out_str.find("has been unset") != std::string::npos);
    std::println("    ✓ Output message correct");
    g.out.str("");  // Clear output

    // Verify database: autorep should be 0
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).autorep == 0);
    std::println("    ✓ Database: autorep = {} (OFF)", saved->info(0).autorep);
  }

  // TEST 3: Toggle back ON again
  std::println("  Testing: Toggle back ON");
  {
    command_t cmd = {"autoreport"};
    GB::commands::autoreport(cmd, g);

    // Verify database: should be ON again
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).autorep == TELEG_MAX_AUTO);
    std::println("    ✓ Database: autorep = {} (ON)", saved->info(0).autorep);
  }

  std::println("  ✅ All autoreport database persistence tests passed!");
}

int main() {
  test_autoreport_database_persistence();
  std::println("\n✅ All tests passed!");
  return 0;
}
