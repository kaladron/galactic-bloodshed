// SPDX-License-Identifier: Apache-2.0

/// \file name_test.cc
/// \brief Test name command database persistence

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

void test_name_ship_persistence() {
  std::println("Test: name command - ship naming");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a ship
  Ship ship{};
  ship.number() = 1;
  ship.name() = "Old Ship Name";

  JsonStore store(ctx.db);
  ShipRepository ships(store);
  ships.save(ship);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_shipno(1);
  g.race =
      ctx.em.peek_race(g.player());  // Set race pointer like production does

  // TEST: Rename ship
  std::println("  Testing: Rename ship to 'USS Enterprise'");
  {
    command_t cmd = {"name", "ship", "USS", "Enterprise"};
    GB::commands::name(cmd, g);

    // Verify output message
    std::string out_str = g.out.str();
    assert(out_str.find("Name set.") != std::string::npos);
    std::println("    ✓ Output message correct");

    // Verify database
    auto saved = ships.find_by_number(1);
    assert(saved.has_value());
    assert(saved->name() == "USS Enterprise");
    std::println("    ✓ Database: ship name = '{}'", saved->name());
  }

  std::println("  ✅ Ship naming test passed!");
}

void test_name_race_persistence() {
  std::println("Test: name command - race naming");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "Old Race Name";

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create GameObj for command execution (leader, not governor)
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_governor(0);  // Must be leader (governor 0)
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.race =
      ctx.em.peek_race(g.player());  // Set race pointer like production does

  // TEST: Rename race
  std::println("  Testing: Rename race to 'Klingons'");
  {
    command_t cmd = {"name", "race", "Klingons"};
    GB::commands::name(cmd, g);

    // Verify database
    auto saved = races.find_by_player(1);
    assert(saved.has_value());
    assert(saved->name == "Klingons");
    std::println("    ✓ Database: race name = '{}'", saved->name);
  }

  std::println("  ✅ Race naming test passed!");
}

void test_name_star_persistence() {
  std::println("Test: name command - star naming");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race (God)
  Race race{};
  race.Playernum = 1;
  race.God = 1;  // Must be deity

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.name = "Old Star Name";
  Star star{star_data};

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  g.race =
      ctx.em.peek_race(g.player());  // Set race pointer like production does

  // TEST: Rename star
  std::println("  Testing: Rename star to 'Alpha Centauri'");
  {
    command_t cmd = {"name", "star", "Alpha", "Centauri"};
    GB::commands::name(cmd, g);

    // Verify database
    auto saved = stars_repo.find_by_number(1);
    assert(saved.has_value());
    assert(saved->get_name() == "Alpha Centauri");
    std::println("    ✓ Database: star name = '{}'", saved->get_name());
  }

  std::println("  ✅ Star naming test passed!");
}

void test_name_planet_persistence() {
  std::println("Test: name command - planet naming");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race (God)
  Race race{};
  race.Playernum = 1;
  race.God = 1;  // Must be deity

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Setup: Create a star with planets
  star_struct star_data{};
  star_data.star_id = 1;
  star_data.name = "Test Star";
  star_data.pnames.push_back("Old Planet Name");
  Star star{star_data};

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  g.race =
      ctx.em.peek_race(g.player());  // Set race pointer like production does

  // TEST: Rename planet
  std::println("  Testing: Rename planet to 'New Earth'");
  {
    command_t cmd = {"name", "planet", "New", "Earth"};
    GB::commands::name(cmd, g);

    // Verify database
    auto saved = stars_repo.find_by_number(1);
    assert(saved.has_value());
    assert(saved->get_planet_name(0) == "New Earth");
    std::println("    ✓ Database: planet name = '{}'",
                 saved->get_planet_name(0));
  }

  std::println("  ✅ Planet naming test passed!");
}

int main() {
  test_name_ship_persistence();
  test_name_race_persistence();
  test_name_star_persistence();
  test_name_planet_persistence();
  std::println("\n✅ All name tests passed!");
  return 0;
}
