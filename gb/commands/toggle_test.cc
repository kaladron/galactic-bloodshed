// SPDX-License-Identifier: Apache-2.0

/// \file toggle_test.cc
/// \brief Test toggle command database persistence

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

void test_toggle_database_persistence() {
  std::println("Test: toggle command database persistence");

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
  std::println("  Testing: Display all toggle settings");
  {
    command_t cmd = {"toggle"};
    GB::commands::toggle(cmd, g);

    // Verify output contains all toggle names
    std::string out_str = g.out.str();
    assert(out_str.find("gag") != std::string::npos);
    assert(out_str.find("inverse") != std::string::npos);
    assert(out_str.find("double_digits") != std::string::npos);
    assert(out_str.find("geography") != std::string::npos);
    assert(out_str.find("autoload") != std::string::npos);
    assert(out_str.find("color") != std::string::npos);
    assert(out_str.find("compatibility") != std::string::npos);
    assert(out_str.find("VISIBLE") != std::string::npos);
    std::println("    ✓ Output displays all toggles");
    g.out.str("");  // Clear output for next test
  }

  // TEST 2: Toggle gag setting
  std::println("  Testing: Toggle gag setting");
  {
    command_t cmd = {"toggle", "gag"};
    GB::commands::toggle(cmd, g);

    // Verify output
    std::string out_str = g.out.str();
    assert(out_str.find("gag is now on") != std::string::npos);
    std::println("    ✓ Output message correct");
    g.out.str("");

    // Verify database: gag should be true
    auto saved = races.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.gag == true);
    std::println("    ✓ Database: gag = true");

    // Toggle again - should turn off
    GB::commands::toggle(cmd, g);
    out_str = g.out.str();
    assert(out_str.find("gag is now off") != std::string::npos);

    saved = races.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.gag == false);
    std::println("    ✓ Database: gag = false after second toggle");
    g.out.str("");
  }

  // TEST 3: Toggle inverse setting
  std::println("  Testing: Toggle inverse setting");
  {
    command_t cmd = {"toggle", "inverse"};
    GB::commands::toggle(cmd, g);

    // Verify database
    auto saved = races.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.inverse == true);
    std::println("    ✓ Database: inverse = true");
    g.out.str("");
  }

  // TEST 4: Toggle double_digits setting
  std::println("  Testing: Toggle double_digits setting");
  {
    command_t cmd = {"toggle", "double_digits"};
    GB::commands::toggle(cmd, g);

    // Verify database
    auto saved = races.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.double_digits == true);
    std::println("    ✓ Database: double_digits = true");
    g.out.str("");
  }

  // TEST 5: Toggle geography setting
  std::println("  Testing: Toggle geography setting");
  {
    command_t cmd = {"toggle", "geography"};
    GB::commands::toggle(cmd, g);

    // Verify database
    auto saved = races.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.geography == true);
    std::println("    ✓ Database: geography = true");
    g.out.str("");
  }

  // TEST 6: Toggle autoload setting
  std::println("  Testing: Toggle autoload setting");
  {
    command_t cmd = {"toggle", "autoload"};
    GB::commands::toggle(cmd, g);

    // Verify database
    auto saved = races.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.autoload == true);
    std::println("    ✓ Database: autoload = true");
    g.out.str("");
  }

  // TEST 7: Toggle color setting
  std::println("  Testing: Toggle color setting");
  {
    command_t cmd = {"toggle", "color"};
    GB::commands::toggle(cmd, g);

    // Verify database
    auto saved = races.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.color == true);
    std::println("    ✓ Database: color = true");
    g.out.str("");
  }

  // TEST 8: Toggle compatibility setting
  std::println("  Testing: Toggle compatibility setting");
  {
    command_t cmd = {"toggle", "compatibility"};
    GB::commands::toggle(cmd, g);

    // Verify database
    auto saved = races.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.compat == true);
    std::println("    ✓ Database: compat = true");
    g.out.str("");
  }

  // TEST 9: Toggle visible setting
  std::println("  Testing: Toggle visible setting");
  {
    command_t cmd = {"toggle", "visible"};
    GB::commands::toggle(cmd, g);

    // Verify database (invisible flag should toggle)
    auto saved = races.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.invisible == true);
    std::println("    ✓ Database: invisible = true");
    g.out.str("");
  }

  // TEST 10: Toggle monitor setting (God only)
  std::println("  Testing: Toggle monitor setting (God mode)");
  {
    // First set race as God
    auto race_handle = ctx.em.get_race(1);
    auto& race_mod = *race_handle;
    race_mod.God = 1;
    // Auto-saves when scope exits

    // Update g.race pointer
    g.race = ctx.em.peek_race(1);

    command_t cmd = {"toggle", "monitor"};
    GB::commands::toggle(cmd, g);

    // Verify database
    auto saved = races.find_by_player(1);
    assert(saved.has_value());
    assert(saved->monitor == true);
    std::println("    ✓ Database: monitor = true (God mode)");
    g.out.str("");
  }

  // TEST 11: Reject invalid toggle option
  std::println("  Testing: Reject invalid toggle option");
  {
    command_t cmd = {"toggle", "invalid_option"};
    GB::commands::toggle(cmd, g);

    // Verify error message
    std::string out_str = g.out.str();
    assert(out_str.find("No such option") != std::string::npos);
    std::println("    ✓ Error message correct");
  }

  std::println("  ✅ All toggle database persistence tests passed!");
}

int main() {
  test_toggle_database_persistence();
  std::println("\n✅ All tests passed!");
  return 0;
}
