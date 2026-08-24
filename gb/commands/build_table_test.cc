// SPDX-License-Identifier: Apache-2.0

/// \file build_table_test.cc
/// \brief Unit tests for "build ?" ship list table display

import dallib;
import gblib;
import test;
import commands;
import std;

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

  // "build ?" displays ship list table
  {
    command_t argv = {"build", "?"};
    GB::commands::build(argv, g);

    std::string output = g.out.str();

    // Display output for visual verification
    std::println(std::cout, "=== build ? output ===");
    std::print("{}", output);
    std::println(std::cout, "=== end output ===");

    // Verify header is present
    test::expect_contains(output, "Default ship parameters");

    // Verify table header columns are present
    test::expect_contains(output, "name");
    test::expect_contains(output, "cargo");
    test::expect_contains(output, "tech");
    test::expect_contains(output, "cost");
    test::expect_contains(output, "crew");
    test::expect_contains(output, "fuel");

    // Verify some common ship types are listed
    test::expect_contains(output, "Probe");
    test::expect_contains(output, "Shuttle");
    test::expect_contains(output, "Factory");

    // Verify table structure (columns should be aligned)
    // The letter column should have single characters
    test::expect_contains(output, ":");  // Probe letter
    test::expect_contains(output, "s");  // Shuttle letter

    std::println(std::cout, "✓ build ? table display test passed");
  }

  // Clear output buffer
  g.out.str("");
  g.out.clear();

  // "build ? :" shows detailed info for probe
  {
    command_t argv = {"build", "?", ":"};
    GB::commands::build(argv, g);

    std::string output = g.out.str();

    // Display output for visual verification
    std::println(std::cout, "\n=== build ? : output ===");
    std::print("{}", output);
    std::println(std::cout, "=== end output ===");

    // Verify it shows probe-specific info
    // The table should show just the probe row
    test::expect_contains(output, "Probe");

    // Should describe where it can be built
    test::expect_contains(output, "Can be");

    std::println(std::cout, "✓ build ? : (single ship) test passed");
  }

  std::println(std::cout, "\n✓ All build table tests passed!");
  return 0;
}
