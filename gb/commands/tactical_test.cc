// SPDX-License-Identifier: Apache-2.0

/// \file tactical_test.cc
/// \brief Test tactical command functionality
///
/// This test verifies the standalone tactical.cc command works correctly.
/// The tactical command shows a combat display of ships and planets in the
/// current scope.

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

// Create a minimal universe with ships for testing
void setup_test_universe(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.tech = 100.0;
  race.mass = 1.0;
  race.metabolism = 1.0;
  race.governor[0].active = true;
  race.governor[0].name = "Governor1";

  RaceRepository races(store);
  races.save(race);

  // Create a star
  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.pnames.push_back("TestPlanet");
  star.explored = 2;     // Player 1 has explored (bit 1 set: 1 << 1 = 2)
  star.inhabited = 2;    // Player 1 inhabits (bit 1 set)
  star.governor[0] = 0;  // Player 1 governor

  StarRepository stars(store);
  stars.save(star);

  // Create a planet
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.popn() = 1000;
  planet.info(player_t{1}).numsectsowned = 10;
  planet.info(player_t{1}).explored = 1;  // Player 1 has explored this planet

  PlanetRepository planets(store);
  planets.save(planet);

  // Create some ships for the player
  // Ship 1: At planet scope
  Ship ship1{};
  ship1.number() = 1;
  ship1.type() = ShipType::OTYPE_FACTORY;
  ship1.owner() = 1;
  ship1.governor() = 0;
  ship1.alive() = true;
  ship1.name() = "Factory1";
  ship1.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship1.storbits() = 1;
  ship1.pnumorbits() = 0;

  ShipRepository ships(store);
  ships.save(ship1);
}

/// Test tactical at planet scope - shows ships orbiting the planet
void test_tactical_planet_scope() {
  std::println(std::cout, "Test: Tactical at planet scope");

  TestContext ctx;
  setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, 1, 0);
  g_tactical.set_level(ScopeLevel::LEVEL_PLAN);
  g_tactical.set_snum(1);
  g_tactical.set_pnum(0);

  ctx.assert_dispatch_success(g_tactical, {"tactical"});
  std::string tactical_output = g_tactical.out.str();

  // Verify tactical produces output
  test::expect_false(tactical_output.empty(),
                     "Tactical should produce output at planet scope");

  // Verify the output mentions the planet
  test::expect_contains(tactical_output, "TestPlanet",
                        "Tactical at planet scope should show planet");

  std::println(std::cout, "  ✓ Planet scope produces tactical output");
}

/// Test tactical at ship scope - shows surrounding area (planet + ships)
void test_tactical_ship_scope() {
  std::println(std::cout, "Test: Tactical at ship scope");

  TestContext ctx;
  setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, 1, 0);
  g_tactical.set_level(ScopeLevel::LEVEL_SHIP);
  g_tactical.set_snum(1);
  g_tactical.set_pnum(0);
  g_tactical.set_shipno(1);

  ctx.assert_dispatch_success(g_tactical, {"tactical"});
  std::string tactical_output = g_tactical.out.str();

  // Verify we got output
  test::expect_false(tactical_output.empty(),
                     "Tactical should produce output at ship scope");

  // Verify the output contains the planet name (showing surrounding area)
  test::expect_contains(
      tactical_output, "TestPlanet",
      "Tactical at ship scope should show surrounding planet");

  std::println(std::cout,
               "  ✓ Ship scope produces tactical output with surrounding area");
}

/// Test tactical at star scope - shows planets and ships in the star system
void test_tactical_star_scope() {
  std::println(std::cout, "Test: Tactical at star scope");

  TestContext ctx;
  setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, 1, 0);
  g_tactical.set_level(ScopeLevel::LEVEL_STAR);
  g_tactical.set_snum(1);
  g_tactical.set_pnum(0);

  ctx.assert_dispatch_success(g_tactical, {"tactical"});
  std::string tactical_output = g_tactical.out.str();

  // Verify tactical produces output
  test::expect_false(tactical_output.empty(),
                     "Tactical should produce output at star scope");

  // Verify the output mentions the planet
  test::expect_contains(tactical_output, "TestPlanet",
                        "Tactical at star scope should show planet");

  std::println(std::cout, "  ✓ Star scope produces tactical output");
}

void test_tactical_scope_rejection() {
  std::println(std::cout, "Test: Tactical scope rejection at UNIV scope");

  TestContext ctx;
  setup_test_universe(ctx);

  auto& registry = get_test_session_registry();
  GameObj g_tactical(ctx.em, registry);
  ctx.setup_game_obj(g_tactical, 1, 0);
  g_tactical.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_rejected(g_tactical, {"tactical"});
  test::expect_contains(g_tactical.out.str(), "Invalid scope for this command");
  std::println(std::cout, "  ✓ Tactical rejected at universe level");
}

}  // namespace

int main() {
  std::println(std::cout, "=== Tactical Command Test ===\n");

  test_tactical_planet_scope();
  test_tactical_ship_scope();
  test_tactical_star_scope();
  test_tactical_scope_rejection();

  std::println(std::cout, "\n✅ All tactical tests passed!");
  return 0;
}
