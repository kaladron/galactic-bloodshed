// SPDX-License-Identifier: Apache-2.0

/// \file autoreport_test.cc
/// \brief Unit tests for autoreport command and database persistence.

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

namespace {

void test_autoreport_database_persistence() {
  std::println(std::cout, "Test: autoreport command database persistence");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a star
  star_struct star{};
  star.star_id = 1;
  star.name = "Test Star";
  star.pnames.push_back("Test Planet");
  star.governor[0] = 0;  // Player 1 (index 0) governor 0

  JsonStore store(ctx.db);
  StarRepository stars(store);
  stars.save(star);

  // Setup: Create a planet with autoreport initially OFF
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.info(player_t{1}).numsectsowned = 10;  // Player 1 owns sectors
  planet.info(player_t{1}).autorep = 0;         // Initially OFF

  PlanetRepository planets(store);
  planets.save(planet);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);

  // 1. Scope rejection at UNIV level
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(1);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"autoreport"});
  assert(g.out.str().contains("Invalid scope for this command."));
  std::println(std::cout, "    ✓ Scope rejection at universe level verified");

  // 2. Star control authorization rejection
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);
  g.set_governor(2);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"autoreport"});
  assert(g.out.str().contains(
      "You are not authorized to do that in this system."));
  std::println(std::cout,
               "    ✓ Star control rejection for governor 2 verified");

  // Restore authorized governor
  g.set_governor(0);

  // TEST 1: Toggle autoreport ON
  std::println(std::cout, "  Testing: Toggle autoreport ON");
  {
    ctx.assert_dispatch_success(g, {"autoreport"});

    // Verify output message
    std::string out_str = g.out.str();
    assert(out_str.find("has been set") != std::string::npos);
    std::println(std::cout, "    ✓ Output message correct");
    g.out.str("");  // Clear output for next test

    // Verify database: autorep should be TELEG_MAX_AUTO (63)
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(player_t{1}).autorep == TELEG_MAX_AUTO);
    std::println(std::cout, "    ✓ Database: autorep = {} (ON)",
                 saved->info(player_t{1}).autorep);
  }

  // TEST 2: Toggle autoreport OFF
  std::println(std::cout, "  Testing: Toggle autoreport OFF");
  {
    ctx.assert_dispatch_success(g, {"autoreport"});

    // Verify output message
    std::string out_str = g.out.str();
    assert(out_str.find("has been unset") != std::string::npos);
    std::println(std::cout, "    ✓ Output message correct");
    g.out.str("");  // Clear output

    // Verify database: autorep should be 0
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(player_t{1}).autorep == 0);
    std::println(std::cout, "    ✓ Database: autorep = {} (OFF)",
                 saved->info(player_t{1}).autorep);
  }

  // TEST 3: Toggle back ON again
  std::println(std::cout, "  Testing: Toggle back ON");
  {
    ctx.assert_dispatch_success(g, {"autoreport"});

    // Verify database: should be ON again
    auto saved = planets.find_by_location(1, 0);
    assert(saved.has_value());
    assert(saved->info(player_t{1}).autorep == TELEG_MAX_AUTO);
    std::println(std::cout, "    ✓ Database: autorep = {} (ON)",
                 saved->info(player_t{1}).autorep);
  }

  std::println(std::cout,
               "  ✅ All autoreport database persistence tests passed!");
}

}  // namespace

int main() {
  test_autoreport_database_persistence();
  std::println(std::cout, "\n✅ All autoreport tests passed!");
  return 0;
}
