// SPDX-License-Identifier: Apache-2.0

/// \file tactical_test.cc
/// \brief Test tactical command functionality
///
/// This test verifies the standalone tactical.cc command works correctly.
/// The tactical command shows a combat display of ships and planets in the
/// current scope.

import dallib;
import gblib;
import commands;
import std;

#include <cassert>
#include <cstddef>

// Setup common game state used by all tests
struct TestState {
  Database db;
  EntityManager em;
  JsonStore store;

  TestState() : db(":memory:"), em(db), store(db) { initialize_schema(db); }
};

// Create a minimal universe with ships for testing
void setup_test_universe(TestState& state) {
  // Create a race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.tech = 100.0;
  race.mass = 1.0;
  race.metabolism = 1.0;
  race.governor[0].active = true;
  race.governor[0].name = "Governor1";

  RaceRepository races(state.store);
  races.save(race);

  // Create a star
  star_struct star{};
  star.star_id = 1;
  star.name = "TestStar";
  star.pnames.push_back("TestPlanet");
  star.explored = 2;     // Player 1 has explored (bit 1 set: 1 << 1 = 2)
  star.inhabited = 2;    // Player 1 inhabits (bit 1 set)
  star.governor[0] = 0;  // Player 1 governor

  StarRepository stars(state.store);
  stars.save(star);

  // Create a planet
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.popn() = 1000;
  planet.info(0).numsectsowned = 10;
  planet.info(0).explored = 1;  // Player 1 has explored this planet

  PlanetRepository planets(state.store);
  planets.save(planet);

  // Create some ships for the player
  // Ship 1: At planet scope
  Ship ship1{};
  ship1.number = 1;
  ship1.type = ShipType::OTYPE_FACTORY;
  ship1.owner = 1;
  ship1.governor = 0;
  ship1.alive = true;
  ship1.name = "Factory1";
  ship1.whatorbits = ScopeLevel::LEVEL_PLAN;
  ship1.storbits = 1;
  ship1.pnumorbits = 0;

  ShipRepository ships(state.store);
  ships.save(ship1);
}

/// Test tactical at planet scope - shows ships orbiting the planet
void test_tactical_planet_scope() {
  std::println("Test: Tactical at planet scope");

  TestState state;
  setup_test_universe(state);

  // Refresh entity manager cache
  state.em.clear_cache();

  // Create GameObj for tactical command
  GameObj g_tactical(state.em);
  g_tactical.player = 1;
  g_tactical.governor = 0;
  g_tactical.level = ScopeLevel::LEVEL_PLAN;
  g_tactical.snum = 1;
  g_tactical.pnum = 0;
  g_tactical.race = state.em.peek_race(1);

  // Run standalone tactical command
  command_t tactical_cmd = {"tactical"};
  GB::commands::tactical(tactical_cmd, g_tactical);
  std::string tactical_output = g_tactical.out.str();

  // Verify tactical produces output
  assert(!tactical_output.empty() && "Tactical should produce output at planet scope");

  // Verify the output mentions the planet
  assert(tactical_output.find("TestPlanet") != std::string::npos &&
         "Tactical at planet scope should show planet");

  std::println("  ✓ Planet scope produces tactical output");
  std::println("  Output:\n{}", tactical_output);
}

/// Test tactical at ship scope - shows surrounding area (planet + ships)
/// Per documentation: "Enemy ships will only appear on tactical display
/// if they are in the same scope as the calling ship."
void test_tactical_ship_scope() {
  std::println("Test: Tactical at ship scope");

  TestState state;
  setup_test_universe(state);

  // Refresh entity manager cache
  state.em.clear_cache();

  // Create GameObj for tactical command
  GameObj g_tactical(state.em);
  g_tactical.player = 1;
  g_tactical.governor = 0;
  g_tactical.level = ScopeLevel::LEVEL_SHIP;
  g_tactical.snum = 1;
  g_tactical.pnum = 0;
  g_tactical.shipno = 1;
  g_tactical.race = state.em.peek_race(1);

  // Run standalone tactical command
  command_t tactical_cmd = {"tactical"};
  GB::commands::tactical(tactical_cmd, g_tactical);
  std::string tactical_output = g_tactical.out.str();

  // Verify we got output
  assert(!tactical_output.empty() && "Tactical should produce output at ship scope");

  // Verify the output contains the planet name (showing surrounding area)
  assert(tactical_output.find("TestPlanet") != std::string::npos &&
         "Tactical at ship scope should show surrounding planet");

  std::println("  ✓ Ship scope produces tactical output with surrounding area");
  std::println("  Output:\n{}", tactical_output);
}

/// Test tactical at star scope - shows planets and ships in the star system
void test_tactical_star_scope() {
  std::println("Test: Tactical at star scope");

  TestState state;
  setup_test_universe(state);

  // Refresh entity manager cache
  state.em.clear_cache();

  // Create GameObj for tactical command
  GameObj g_tactical(state.em);
  g_tactical.player = 1;
  g_tactical.governor = 0;
  g_tactical.level = ScopeLevel::LEVEL_STAR;
  g_tactical.snum = 1;
  g_tactical.pnum = 0;
  g_tactical.race = state.em.peek_race(1);

  // Run standalone tactical command
  command_t tactical_cmd = {"tactical"};
  GB::commands::tactical(tactical_cmd, g_tactical);
  std::string tactical_output = g_tactical.out.str();

  // Verify tactical produces output
  assert(!tactical_output.empty() && "Tactical should produce output at star scope");

  // Verify the output mentions the planet
  assert(tactical_output.find("TestPlanet") != std::string::npos &&
         "Tactical at star scope should show planet");

  std::println("  ✓ Star scope produces tactical output");
  std::println("  Output:\n{}", tactical_output);
}

int main() {
  std::println("=== Tactical Command Test ===\n");

  test_tactical_planet_scope();
  test_tactical_ship_scope();
  test_tactical_star_scope();

  std::println("\n✅ All tactical tests passed!");
  return 0;
}
