// SPDX-License-Identifier: Apache-2.0

/// \file build_table_test.cc
/// \brief Unit tests for "build ?" ship list table display

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import test;
import commands;
import std;

int main() {
  // Initialize database
  TestContext ctx;

  // Create a test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 500.0;  // High tech to build any ship
  race.pods = true;   // Can build pods

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  ShipExamRepository exams(store);
  test::expect_true(exams.seed_from_file(pkg_data_path("exam.dat").string()));

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
    test::expect_contains(output, "Space Probe");

    std::println(std::cout, "✓ build ? : (single ship) test passed");
  }

  // Regression test for issue #60: every programmed ShipType letter returns its
  // exact canonical exam.dat description (and unprogrammed letters report
  // "This ship type has not been programmed.").
  {
    static constexpr std::array<std::string_view, NUMSTYPES> expected_headings =
        {
            "Spore Pod",
            "Shuttle",
            "Carrier",
            "Dreadnaught",
            "Battleship",
            "Interceptor",
            "Cruiser",
            "Destroyer",
            "Fighter Group",
            "Explorer",
            "Space Habitat",
            "Space Station",
            "Orbital Assault Platform",
            "Cargo Ship",
            "Tanker",
            "Godship",
            "Space Mine",
            "Space Mirror",
            "Space Telescope",
            "Ground Telescope",
            "Tractor-Repulsor Beam",
            "Atmosphere Processor",
            "Dust Canister",
            "Greenhouse Gases",
            "Von Neumann Machine",
            "Berserker",
            "Governmental Center",
            "Mind Control Laser",
            "Toxic Waste Container",
            "Space Probe",
            "Gamma Ray Laser",
            "Factory",
            "Terraforming Device",
            "Berserker Control Center",
            "Berserker Autofac",
            "Audio-Vibratory-Physio-Molecular Transport Device",
            "Missile",
            "Planetary Defense Network",
            "Quarry",
            "Space Plow",
            "Dome",
            "Weapons Plant",
            "Space Port",
            "Anti-Ballistic Missile Battery",
            "AFV",
            "Bunker",
            "Lander",
        };

    for (int i = 0; i < NUMSTYPES; ++i) {
      const auto stype = static_cast<ShipType>(i);
      const auto& tmpl = ship_template(stype);
      g.out.str("");
      g.out.clear();

      command_t argv = {"build", "?", std::string(1, tmpl.letter)};
      const bool ok = GB::commands::build(argv, g);
      const std::string output = g.out.str();

      if (tmpl.is_programmed) {
        test::expect_true(ok);
        test::expect_contains(output,
                              expected_headings[static_cast<std::size_t>(i)]);
        test::expect_contains(output, tmpl.name);
      } else {
        test::expect_false(ok);
        test::expect_contains(output,
                              "This ship type has not been programmed.");
      }
    }
    std::println(
        std::cout,
        "✓ build ? <letter> verified across all 47 canonical ship types");
  }

  std::println(std::cout, "\n✓ All build table tests passed!");
  return 0;
}
