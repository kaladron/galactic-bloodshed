// SPDX-License-Identifier: Apache-2.0

/// \file tactical_test.cc
/// \brief Test tactical command functionality
///
/// This test verifies the standalone tactical.cc command works correctly.
/// Note: The new tactical.cc has improved ship-scope behavior compared to
/// rst.cc's tactical mode - it shows the surrounding area (planet + ships)
/// per the documentation: "Enemy ships will only appear on tactical display
/// if they are in the same scope as the calling ship."

import dallib;
import gblib;
import commands;
import std;

#include <cassert>
#include <cstddef>

// Helper to split a string into lines for comparison
std::vector<std::string> split_lines(const std::string& s) {
  std::vector<std::string> result;
  std::istringstream stream(s);
  std::string line;
  while (std::getline(stream, line)) {
    result.push_back(line);
  }
  return result;
}

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
  star.explored = 1;     // Player 1 has explored
  star.inhabited = 1;    // Player 1 inhabits
  star.governor[0] = 0;  // Player 1 governor

  StarRepository stars(state.store);
  stars.save(star);

  // Create a planet
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.popn() = 1000;
  planet.info(0).numsectsowned = 10;

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

/// Test tactical at planet scope - should match rst tactical
void test_tactical_planet_scope() {
  std::println("Test: Tactical at planet scope");

  TestState state;
  setup_test_universe(state);

  // Refresh entity manager cache
  state.em.clear_cache();

  // Create GameObj for rst command
  GameObj g_rst(state.em);
  g_rst.player = 1;
  g_rst.governor = 0;
  g_rst.level = ScopeLevel::LEVEL_PLAN;
  g_rst.snum = 1;
  g_rst.pnum = 0;
  g_rst.race = state.em.peek_race(1);

  // Create GameObj for tactical command
  GameObj g_tactical(state.em);
  g_tactical.player = 1;
  g_tactical.governor = 0;
  g_tactical.level = ScopeLevel::LEVEL_PLAN;
  g_tactical.snum = 1;
  g_tactical.pnum = 0;
  g_tactical.race = state.em.peek_race(1);

  // Run rst tactical mode
  command_t rst_cmd = {"tactical"};
  GB::commands::rst(rst_cmd, g_rst);
  std::string rst_output = g_rst.out.str();

  // Run standalone tactical command
  command_t tactical_cmd = {"tactical"};
  GB::commands::tactical(tactical_cmd, g_tactical);
  std::string tactical_output = g_tactical.out.str();

  // Compare outputs - at planet scope they should match
  if (rst_output == tactical_output) {
    std::println("  ✓ Planet scope outputs match exactly");
  } else {
    std::println("  ✗ Planet scope outputs differ!");
    std::println("\n--- RST OUTPUT (planet scope) ---");
    std::println("{}", rst_output);
    std::println("\n--- TACTICAL OUTPUT (planet scope) ---");
    std::println("{}", tactical_output);

    // Show line-by-line diff
    auto rst_lines = split_lines(rst_output);
    auto tactical_lines = split_lines(tactical_output);

    std::println("\n--- Line-by-line comparison ---");
    std::size_t max_lines = std::max(rst_lines.size(), tactical_lines.size());

    for (std::size_t i = 0; i < max_lines; ++i) {
      std::string rst_line =
          (i < rst_lines.size()) ? rst_lines[i] : "(missing)";
      std::string tac_line =
          (i < tactical_lines.size()) ? tactical_lines[i] : "(missing)";
      if (rst_line != tac_line) {
        std::println("Line {}: DIFFER", i);
        std::println("  RST: [{}]", rst_line);
        std::println("  TAC: [{}]", tac_line);
      }
    }
    assert(false && "Planet scope outputs must match");
  }
}

/// Test tactical at ship scope
/// Note: tactical.cc has IMPROVED behavior here - it shows the surrounding
/// area (planet + ships) per documentation, while rst.cc only shows the ship.
void test_tactical_ship_scope() {
  std::println("Test: Tactical at ship scope (improved behavior)");

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

  // Verify we got output (not empty like rst.cc at ship scope)
  assert(!tactical_output.empty() && "Tactical should produce output at ship scope");

  // Verify the output contains the planet name (showing surrounding area)
  assert(tactical_output.find("TestPlanet") != std::string::npos &&
         "Tactical at ship scope should show surrounding planet");

  std::println("  ✓ Ship scope produces tactical output with surrounding area");
  std::println("  Output:\n{}", tactical_output);
}

/// Test tactical at star scope - should match rst tactical
void test_tactical_star_scope() {
  std::println("Test: Tactical at star scope");

  TestState state;
  setup_test_universe(state);

  // Refresh entity manager cache
  state.em.clear_cache();

  // Create GameObj for rst command
  GameObj g_rst(state.em);
  g_rst.player = 1;
  g_rst.governor = 0;
  g_rst.level = ScopeLevel::LEVEL_STAR;
  g_rst.snum = 1;
  g_rst.pnum = 0;
  g_rst.race = state.em.peek_race(1);

  // Create GameObj for tactical command
  GameObj g_tactical(state.em);
  g_tactical.player = 1;
  g_tactical.governor = 0;
  g_tactical.level = ScopeLevel::LEVEL_STAR;
  g_tactical.snum = 1;
  g_tactical.pnum = 0;
  g_tactical.race = state.em.peek_race(1);

  // Run rst tactical mode
  command_t rst_cmd = {"tactical"};
  GB::commands::rst(rst_cmd, g_rst);
  std::string rst_output = g_rst.out.str();

  // Run standalone tactical command
  command_t tactical_cmd = {"tactical"};
  GB::commands::tactical(tactical_cmd, g_tactical);
  std::string tactical_output = g_tactical.out.str();

  // Compare outputs - at star scope they should match
  if (rst_output == tactical_output) {
    std::println("  ✓ Star scope outputs match exactly");
  } else {
    std::println("  ✗ Star scope outputs differ!");
    std::println("\n--- RST OUTPUT (star scope) ---");
    std::println("{}", rst_output);
    std::println("\n--- TACTICAL OUTPUT (star scope) ---");
    std::println("{}", tactical_output);

    // Show line-by-line diff
    auto rst_lines = split_lines(rst_output);
    auto tactical_lines = split_lines(tactical_output);

    std::println("\n--- Line-by-line comparison ---");
    std::size_t max_lines = std::max(rst_lines.size(), tactical_lines.size());

    for (std::size_t i = 0; i < max_lines; ++i) {
      std::string rst_line =
          (i < rst_lines.size()) ? rst_lines[i] : "(missing)";
      std::string tac_line =
          (i < tactical_lines.size()) ? tactical_lines[i] : "(missing)";
      if (rst_line != tac_line) {
        std::println("Line {}: DIFFER", i);
        std::println("  RST: [{}]", rst_line);
        std::println("  TAC: [{}]", tac_line);
      }
    }
    assert(false && "Star scope outputs must match");
  }
}

int main() {
  std::println("=== Tactical Command Comparison Test ===\n");

  test_tactical_planet_scope();
  test_tactical_ship_scope();
  test_tactical_star_scope();

  std::println("\n✅ All tactical comparison tests passed!");
  return 0;
}
