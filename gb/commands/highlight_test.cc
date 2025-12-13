// SPDX-License-Identifier: Apache-2.0

/// \file highlight_test.cc
/// \brief Test highlight command database persistence

import dallib;
import gblib;
import commands;
import std;

#include <cassert>

void test_highlight_database_persistence() {
  std::println("Test: highlight command database persistence");

  // Create in-memory database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Setup: Create two races (player 1 and player 2)
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Player 1";
  race1.governor[0].toggle.highlight = 0;  // Initially no highlight

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Player 2";

  JsonStore store(db);
  RaceRepository races_repo(store);
  races_repo.save(race1);
  races_repo.save(race2);

  // Create GameObj for command execution
  GameObj g(em);
  g.player = 1;
  g.governor = 0;

  // TEST 1: Set highlight to player 2
  std::println("  Testing: Set highlight to player 2");
  {
    command_t cmd = {"highlight", "2"};
    GB::commands::highlight(cmd, g);

    // Verify database: highlight should be set to 2
    auto saved = races_repo.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.highlight == 2);
    std::println("    ✓ Database: highlight = {}",
                 saved->governor[0].toggle.highlight);
  }

  // TEST 2: Change highlight to player 1 (self)
  std::println("  Testing: Change highlight to player 1 (self)");
  {
    command_t cmd = {"highlight", "1"};
    GB::commands::highlight(cmd, g);

    auto saved = races_repo.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.highlight == 1);
    std::println("    ✓ Database: highlight = {}",
                 saved->governor[0].toggle.highlight);
  }

  // TEST 3: Change back to player 2
  std::println("  Testing: Change back to player 2");
  {
    command_t cmd = {"highlight", "2"};
    GB::commands::highlight(cmd, g);

    auto saved = races_repo.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.highlight == 2);
    std::println("    ✓ Database: highlight = {}",
                 saved->governor[0].toggle.highlight);
  }

  // TEST 4: Invalid player number
  std::println("  Testing: Invalid player number");
  {
    command_t cmd = {"highlight", "999"};
    GB::commands::highlight(cmd, g);

    std::string out_str = g.out.str();
    assert(out_str.find("No such player") != std::string::npos);
    std::println("    ✓ Error message for invalid player");
    g.out.str("");

    // Verify highlight wasn't changed
    auto saved = races_repo.find_by_player(1);
    assert(saved.has_value());
    assert(saved->governor[0].toggle.highlight == 2);  // Should still be 2
  }

  std::println("  ✅ All highlight database persistence tests passed!");
}

int main() {
  test_highlight_database_persistence();
  std::println("\n✅ All tests passed!");
  return 0;
}
