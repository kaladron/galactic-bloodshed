// SPDX-License-Identifier: Apache-2.0

/// \file toggle_test.cc
/// \brief Test toggle command database persistence

import dallib;
import gblib;
import test;
import commands;
import std;

void test_toggle_database_persistence() {
  std::println(std::cout, "Test: toggle command database persistence");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a race
  Race race{};
  race.Playernum = 1;
  race.Guest = 0;  // Not a guest
  race.God = 0;    // Not God initially
  race.governor[0].toggle.gag = false;
  race.governor[0].toggle.inverse = false;
  race.governor[0].toggle.double_digits = false;
  race.governor[0].toggle.geography = false;
  race.governor[0].toggle.autoload = false;
  race.governor[0].toggle.color = false;
  race.governor[0].toggle.compat = false;
  race.governor[0].toggle.invisible = false;
  race.monitor = false;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);  // Set race pointer like production does

  // TEST 1: Display all toggle settings (no argument)
  std::println(std::cout, "  Testing: Display all toggle settings");
  {
    ctx.assert_dispatch_success(g, {"toggle"});

    // Verify output contains all toggle names
    std::string out_str = g.out.str();
    test::expect_contains(out_str, "gag");
    test::expect_contains(out_str, "inverse");
    test::expect_contains(out_str, "double_digits");
    test::expect_contains(out_str, "geography");
    test::expect_contains(out_str, "autoload");
    test::expect_contains(out_str, "color");
    test::expect_contains(out_str, "compatibility");
    test::expect_contains(out_str, "VISIBLE");
    std::println(std::cout, "    ✓ Output displays all toggles");
    g.out.str("");  // Clear output for next test
  }

  // TEST 2: Toggle gag setting
  std::println(std::cout, "  Testing: Toggle gag setting");
  {
    ctx.assert_dispatch_success(g, {"toggle", "gag"});

    // Verify output
    std::string out_str = g.out.str();
    test::expect_contains(out_str, "gag is now on");
    std::println(std::cout, "    ✓ Output message correct");
    g.out.str("");

    // Verify database: gag should be true
    auto saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_true(saved->governor[0].toggle.gag);
    std::println(std::cout, "    ✓ Database: gag = true");

    // Toggle again - should turn off
    ctx.assert_dispatch_success(g, {"toggle", "gag"});
    out_str = g.out.str();
    test::expect_contains(out_str, "gag is now off");

    saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_false(saved->governor[0].toggle.gag);
    std::println(std::cout, "    ✓ Database: gag = false after second toggle");
    g.out.str("");
  }

  // TEST 3: Toggle inverse setting
  std::println(std::cout, "  Testing: Toggle inverse setting");
  {
    ctx.assert_dispatch_success(g, {"toggle", "inverse"});

    // Verify database
    auto saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_true(saved->governor[0].toggle.inverse);
    std::println(std::cout, "    ✓ Database: inverse = true");
    g.out.str("");
  }

  // TEST 4: Toggle double_digits setting
  std::println(std::cout, "  Testing: Toggle double_digits setting");
  {
    ctx.assert_dispatch_success(g, {"toggle", "double_digits"});

    // Verify database
    auto saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_true(saved->governor[0].toggle.double_digits);
    std::println(std::cout, "    ✓ Database: double_digits = true");
    g.out.str("");
  }

  // TEST 5: Toggle geography setting
  std::println(std::cout, "  Testing: Toggle geography setting");
  {
    ctx.assert_dispatch_success(g, {"toggle", "geography"});

    // Verify database
    auto saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_true(saved->governor[0].toggle.geography);
    std::println(std::cout, "    ✓ Database: geography = true");
    g.out.str("");
  }

  // TEST 6: Toggle autoload setting
  std::println(std::cout, "  Testing: Toggle autoload setting");
  {
    ctx.assert_dispatch_success(g, {"toggle", "autoload"});

    // Verify database
    auto saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_true(saved->governor[0].toggle.autoload);
    std::println(std::cout, "    ✓ Database: autoload = true");
    g.out.str("");
  }

  // TEST 7: Toggle color setting
  std::println(std::cout, "  Testing: Toggle color setting");
  {
    ctx.assert_dispatch_success(g, {"toggle", "color"});

    // Verify database
    auto saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_true(saved->governor[0].toggle.color);
    std::println(std::cout, "    ✓ Database: color = true");
    g.out.str("");
  }

  // TEST 8: Toggle compatibility setting
  std::println(std::cout, "  Testing: Toggle compatibility setting");
  {
    ctx.assert_dispatch_success(g, {"toggle", "compatibility"});

    // Verify database
    auto saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_true(saved->governor[0].toggle.compat);
    std::println(std::cout, "    ✓ Database: compat = true");
    g.out.str("");
  }

  // TEST 9: Toggle visible setting
  std::println(std::cout, "  Testing: Toggle visible setting");
  {
    ctx.assert_dispatch_success(g, {"toggle", "visible"});

    // Verify database (invisible flag should toggle)
    auto saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_true(saved->governor[0].toggle.invisible);
    std::println(std::cout, "    ✓ Database: invisible = true");
    g.out.str("");
  }

  // TEST 10: Toggle monitor setting (God only)
  std::println(std::cout, "  Testing: Toggle monitor setting (God mode)");
  {
    // First set race as God
    auto race_handle = ctx.em.get_race(1);
    auto& race_mod = *race_handle;
    race_mod.God = 1;
    // Auto-saves when scope exits

    // Update g.race pointer
    g.race = ctx.em.peek_race(1);

    ctx.assert_dispatch_success(g, {"toggle", "monitor"});

    // Verify database
    auto saved = races.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_true(saved->monitor);
    std::println(std::cout, "    ✓ Database: monitor = true (God mode)");
    g.out.str("");
  }

  // TEST 11: Reject invalid toggle option
  std::println(std::cout, "  Testing: Reject invalid toggle option");
  {
    ctx.assert_dispatch_rejected(g, {"toggle", "invalid_option"});

    // Verify error message
    std::string out_str = g.out.str();
    test::expect_contains(out_str, "No such option");
    std::println(std::cout, "    ✓ Error message correct");
  }

  std::println(std::cout, "  ✅ All toggle database persistence tests passed!");
}

int main() {
  test_toggle_database_persistence();
  std::println(std::cout, "\n✅ All tests passed!");
  return 0;
}
