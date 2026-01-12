// SPDX-License-Identifier: Apache-2.0

/// \file tax_test.cc
/// \brief Test tax command database persistence

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

void test_tax_database_persistence() {
  std::println("Test: tax command database persistence");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race with government ship
  Race race{};
  race.Playernum = 1;
  race.Gov_ship = 100;  // Has government center
  race.Guest = 0;       // Not a guest

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[0] = 0;  // Player 1 governor 0
  Star star{star_data};

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Setup: Create a planet with initial tax rate
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(player_t{1}).tax = 10;     // Current tax rate
  planet.info(player_t{1}).newtax = 10;  // Target tax rate

  PlanetRepository planets(store);
  planets.save(planet);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  g.race =
      ctx.em.peek_race(g.player());  // Set race pointer like production does

  // TEST 1: Display current tax rate (no argument)
  std::println("  Testing: Display current tax rate");
  {
    command_t cmd = {"tax"};
    GB::commands::tax(cmd, g);

    // Verify output message
    std::string out_str = g.out.str();
    assert(out_str.find("Current tax rate: 10%") != std::string::npos);
    assert(out_str.find("Target: 10%") != std::string::npos);
    std::println("    ✓ Output message correct");
    g.out.str("");  // Clear output for next test
  }

  // TEST 2: Set new tax rate to 25%
  std::println("  Testing: Set tax rate to 25%");
  {
    command_t cmd = {"tax", "25"};
    GB::commands::tax(cmd, g);

    // Verify output message
    std::string out_str = g.out.str();
    assert(out_str.find("Set.") != std::string::npos);
    std::println("    ✓ Output message correct");
    g.out.str("");  // Clear output

    // Verify database: newtax should be 25
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(player_t{1}).newtax == 25);
    std::println("    ✓ Database: newtax = {}",
                 saved->info(player_t{1}).newtax);
  }

  // TEST 3: Set tax rate to maximum (100%)
  std::println("  Testing: Set tax rate to 100%");
  {
    command_t cmd = {"tax", "100"};
    GB::commands::tax(cmd, g);

    // Verify database
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(player_t{1}).newtax == 100);
    std::println("    ✓ Database: newtax = {}",
                 saved->info(player_t{1}).newtax);
    g.out.str("");  // Clear output
  }

  // TEST 4: Set tax rate to minimum (0%)
  std::println("  Testing: Set tax rate to 0%");
  {
    command_t cmd = {"tax", "0"};
    GB::commands::tax(cmd, g);

    // Verify database
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(player_t{1}).newtax == 0);
    std::println("    ✓ Database: newtax = {}",
                 saved->info(player_t{1}).newtax);
    g.out.str("");  // Clear output
  }

  // TEST 5: Reject illegal value (>100)
  std::println("  Testing: Reject illegal value 150%");
  {
    command_t cmd = {"tax", "150"};
    GB::commands::tax(cmd, g);

    // Verify error message
    std::string out_str = g.out.str();
    assert(out_str.find("Illegal value") != std::string::npos);
    std::println("    ✓ Error message correct");

    // Verify database: should still be 0 from previous test
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(player_t{1}).newtax == 0);
    std::println("    ✓ Database: newtax unchanged = {}",
                 saved->info(player_t{1}).newtax);
  }

  std::println("  ✅ All tax database persistence tests passed!");
}

int main() {
  test_tax_database_persistence();
  std::println("\n✅ All tests passed!");
  return 0;
}
