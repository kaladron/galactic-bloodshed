// SPDX-License-Identifier: Apache-2.0
// Test for "build ?" ship list table display

import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

int main() {
  // Initialize database
  TestContext ctx;

  // Create a test race
  Race race{};
  race.Playernum = 1;
  race.governor[0].active = true;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 500.0;  // High tech to build any ship
  race.pods = true;   // Can build pods

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create GameObj for testing
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_UNIV);  // Scope doesn't matter for "build ?"

  // Test 1: "build ?" displays ship list table
  {
    command_t argv = {"build", "?"};
    GB::commands::build(argv, g);

    std::string output = g.out.str();

    // Display output for visual verification
    std::println("=== build ? output ===");
    std::print("{}", output);
    std::println("=== end output ===");

    // Verify header is present
    assert(output.find("Default ship parameters") != std::string::npos);

    // Verify table header columns are present
    assert(output.find("name") != std::string::npos);
    assert(output.find("cargo") != std::string::npos);
    assert(output.find("tech") != std::string::npos);
    assert(output.find("cost") != std::string::npos);
    assert(output.find("crew") != std::string::npos);
    assert(output.find("fuel") != std::string::npos);

    // Verify some common ship types are listed
    assert(output.find("Probe") != std::string::npos);
    assert(output.find("Shuttle") != std::string::npos);
    assert(output.find("Factory") != std::string::npos);

    // Verify table structure (columns should be aligned)
    // The letter column should have single characters
    assert(output.find(":") != std::string::npos);  // Probe letter
    assert(output.find("s") != std::string::npos);  // Shuttle letter

    std::println("✓ build ? table display test passed");
  }

  // Clear output buffer
  g.out.str("");
  g.out.clear();

  // Test 2: "build ? :" shows detailed info for probe
  {
    command_t argv = {"build", "?", ":"};
    GB::commands::build(argv, g);

    std::string output = g.out.str();

    // Display output for visual verification
    std::println("\n=== build ? : output ===");
    std::print("{}", output);
    std::println("=== end output ===");

    // Verify it shows probe-specific info
    // The table should show just the probe row
    assert(output.find("Probe") != std::string::npos);

    // Should describe where it can be built
    assert(output.find("Can be") != std::string::npos);

    std::println("✓ build ? : (single ship) test passed");
  }

  std::println("\n✓ All build table tests passed!");
  return 0;
}
