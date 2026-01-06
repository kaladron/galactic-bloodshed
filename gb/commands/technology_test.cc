// SPDX-License-Identifier: Apache-2.0

/// \file technology_test.cc
/// \brief Test technology command database persistence

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

void test_technology_database_persistence() {
  std::println("Test: technology command database persistence");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;
  race.Guest = 0;  // Not a guest

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[0] = 0;  // Player 1 governor 0
  star_data.AP[0] = 10;       // Enough APs
  Star star{star_data};

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Setup: Create a planet with initial tech investment
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(0).tech_invest = 100;  // Initial investment
  planet.info(0).popn = 1000;        // Population for tech production calc

  PlanetRepository planets(store);
  planets.save(planet);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // TEST 1: Display current tech investment (no argument)
  std::println("  Testing: Display current tech investment");
  {
    command_t cmd = {"technology"};
    GB::commands::technology(cmd, g);

    // Verify output message
    std::string out_str = g.out.str();
    assert(out_str.find("Current investment : 100") != std::string::npos);
    assert(out_str.find("Technology production") != std::string::npos);
    std::println("    ✓ Output message correct");
    g.out.str("");  // Clear output for next test
  }

  // TEST 2: Set tech investment to 500
  std::println("  Testing: Set tech investment to 500");
  {
    command_t cmd = {"technology", "500"};
    GB::commands::technology(cmd, g);

    // Verify output message
    std::string out_str = g.out.str();
    assert(out_str.find("New (ideal) tech production") != std::string::npos);
    std::println("    ✓ Output message correct");
    g.out.str("");

    // Verify database: tech_invest should be 500
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).tech_invest == 500);
    std::println("    ✓ Database: tech_invest = {}",
                 saved->info(0).tech_invest);
  }

  // TEST 3: Set tech investment to 0
  std::println("  Testing: Set tech investment to 0");
  {
    command_t cmd = {"technology", "0"};
    GB::commands::technology(cmd, g);

    // Verify database
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).tech_invest == 0);
    std::println("    ✓ Database: tech_invest = {}",
                 saved->info(0).tech_invest);
    g.out.str("");
  }

  // TEST 4: Set tech investment to large value
  std::println("  Testing: Set tech investment to 10000");
  {
    command_t cmd = {"technology", "10000"};
    GB::commands::technology(cmd, g);

    // Verify database
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).tech_invest == 10000);
    std::println("    ✓ Database: tech_invest = {}",
                 saved->info(0).tech_invest);
    g.out.str("");
  }

  // TEST 5: Reject illegal negative value
  std::println("  Testing: Reject illegal negative value");
  {
    command_t cmd = {"technology", "-100"};
    GB::commands::technology(cmd, g);

    // Verify error message
    std::string out_str = g.out.str();
    assert(out_str.find("Illegal value") != std::string::npos);
    std::println("    ✓ Error message correct");

    // Verify database: should still be 10000 from previous test
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).tech_invest == 10000);
    std::println("    ✓ Database: tech_invest unchanged = {}",
                 saved->info(0).tech_invest);
    g.out.str("");
  }

  // TEST 6: Verify wrong scope level is rejected
  std::println("  Testing: Reject wrong scope level");
  {
    g.set_level(ScopeLevel::LEVEL_UNIV);  // Wrong scope
    command_t cmd = {"technology", "100"};
    GB::commands::technology(cmd, g);

    // Verify error message
    std::string out_str = g.out.str();
    assert(out_str.find("scope must be a planet") != std::string::npos);
    std::println("    ✓ Error message correct for wrong scope");

    // Verify database: should still be 10000 (no change)
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).tech_invest == 10000);
    std::println("    ✓ Database: tech_invest unchanged = {}",
                 saved->info(0).tech_invest);

    // Restore correct scope for further tests
    g.set_level(ScopeLevel::LEVEL_PLAN);
    g.out.str("");
  }

  // TEST 7: Verify unauthorized governor is rejected
  std::println("  Testing: Reject unauthorized governor");
  {
    // Change star governor to 1 (not matching g.governor() = 0)
    auto star_handle = ctx.em.get_star(1);
    auto& star_mod = *star_handle;
    star_mod.governor(0) = 1;  // Different governor

    // Update GameObj governor to non-zero
    g.set_governor(2);  // Not matching star's governor[0] = 1

    command_t cmd = {"technology", "200"};
    GB::commands::technology(cmd, g);

    // Verify error message
    std::string out_str = g.out.str();
    assert(out_str.find("not authorized") != std::string::npos);
    std::println("    ✓ Error message correct for unauthorized governor");

    // Verify database: should still be 10000 (no change)
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).tech_invest == 10000);
    std::println("    ✓ Database: tech_invest unchanged = {}",
                 saved->info(0).tech_invest);

    // Restore for cleanup
    g.set_governor(0);
    g.out.str("");
  }

  std::println("  ✅ All technology database persistence tests passed!");
}

int main() {
  test_technology_database_persistence();
  std::println("\n✅ All tests passed!");
  return 0;
}
