// SPDX-License-Identifier: Apache-2.0

/// \file mobilize_test.cc
/// \brief Test mobilize command database persistence

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

void test_mobilize_database_persistence() {
  std::println("Test: mobilize command database persistence");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a star
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.governor[0] = 0;  // Player 1 governor 0
  star_data.AP[0] = 10;       // Give player 1 some action points
  Star star{star_data};

  JsonStore store(ctx.db);
  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Setup: Create a planet with initial mobilization
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(0).comread = 20;  // Current mobilization
  planet.info(0).mob_set = 20;  // Mobilization quota

  PlanetRepository planets(store);
  planets.save(planet);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // TEST 1: Display current mobilization (no argument)
  std::println("  Testing: Display current mobilization");
  {
    command_t cmd = {"mobilize"};
    GB::commands::mobilize(cmd, g);

    // Note: mobilize uses notify() which doesn't go to g.out in test context
    std::println("    ✓ Command executed (notification sent)");
  }

  // TEST 2: Set new mobilization to 50%
  std::println("  Testing: Set mobilization to 50%");
  {
    command_t cmd = {"mobilize", "50"};
    GB::commands::mobilize(cmd, g);

    g.out.str("");  // Clear for next test

    // Verify database: mob_set should be 50
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).mob_set == 50);
    std::println("    ✓ Database: mob_set = {}", saved->info(0).mob_set);
  }

  // TEST 3: Set mobilization to maximum (100%)
  std::println("  Testing: Set mobilization to 100%");
  {
    command_t cmd = {"mobilize", "100"};
    GB::commands::mobilize(cmd, g);

    // Verify database
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).mob_set == 100);
    std::println("    ✓ Database: mob_set = {}", saved->info(0).mob_set);
  }

  // TEST 4: Set mobilization to minimum (0%)
  std::println("  Testing: Set mobilization to 0%");
  {
    command_t cmd = {"mobilize", "0"};
    GB::commands::mobilize(cmd, g);

    // Verify database
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).mob_set == 0);
    std::println("    ✓ Database: mob_set = {}", saved->info(0).mob_set);
  }

  // TEST 5: Reject illegal value (>100)
  std::println("  Testing: Reject illegal value 150%");
  {
    command_t cmd = {"mobilize", "150"};
    GB::commands::mobilize(cmd, g);

    // Verify error message
    std::string out_str = g.out.str();
    assert(out_str.find("Illegal value") != std::string::npos);
    std::println("    ✓ Error message correct");

    // Verify database: should still be 0 from previous test
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(0).mob_set == 0);
    std::println("    ✓ Database: mob_set unchanged = {}",
                 saved->info(0).mob_set);
  }

  std::println("  ✅ All mobilize database persistence tests passed!");
}

int main() {
  test_mobilize_database_persistence();
  std::println("\n✅ All tests passed!");
  return 0;
}
