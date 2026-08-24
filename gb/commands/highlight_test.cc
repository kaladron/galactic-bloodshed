// SPDX-License-Identifier: Apache-2.0

/// \file highlight_test.cc
/// \brief Test highlight command database persistence

import dallib;
import gblib;
import test;
import commands;
import std;

void test_highlight_database_persistence() {
  std::println(std::cout, "Test: highlight command database persistence");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create two races (player 1 and player 2)
  Race race1{};
  race1.Playernum = 1;
  race1.name = "Player 1";
  race1.governor[0].toggle.highlight = 0;  // Initially no highlight

  Race race2{};
  race2.Playernum = 2;
  race2.name = "Player 2";

  JsonStore store(ctx.db);
  RaceRepository races_repo(store);
  races_repo.save(race1);
  races_repo.save(race2);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  // TEST 1: Set highlight to player 2
  std::println(std::cout, "  Testing: Set highlight to player 2");
  {
    ctx.assert_dispatch_success(g, {"highlight", "2"});

    // Verify database: highlight should be set to 2
    auto saved = races_repo.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_eq(saved->governor[0].toggle.highlight, 2);
    std::println(std::cout, "    ✓ Database: highlight = {}",
                 saved->governor[0].toggle.highlight);
  }

  // TEST 2: Change highlight to player 1 (self)
  std::println(std::cout, "  Testing: Change highlight to player 1 (self)");
  {
    ctx.assert_dispatch_success(g, {"highlight", "1"});

    auto saved = races_repo.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_eq(saved->governor[0].toggle.highlight, 1);
    std::println(std::cout, "    ✓ Database: highlight = {}",
                 saved->governor[0].toggle.highlight);
  }

  // TEST 3: Change back to player 2
  std::println(std::cout, "  Testing: Change back to player 2");
  {
    ctx.assert_dispatch_success(g, {"highlight", "2"});

    auto saved = races_repo.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_eq(saved->governor[0].toggle.highlight, 2);
    std::println(std::cout, "    ✓ Database: highlight = {}",
                 saved->governor[0].toggle.highlight);
  }

  // TEST 4: Invalid player number
  std::println(std::cout, "  Testing: Invalid player number");
  {
    ctx.assert_dispatch_rejected(g, {"highlight", "999"});

    std::string out_str = g.out.str();
    test::expect_contains(out_str, "No such player");
    std::println(std::cout, "    ✓ Error message for invalid player");
    g.out.str("");

    // Verify highlight wasn't changed
    auto saved = races_repo.find_by_player(1);
    test::expect_true(saved.has_value());
    test::expect_eq(saved->governor[0].toggle.highlight,
                    2);  // Should still be 2
  }

  std::println(std::cout,
               "  ✅ All highlight database persistence tests passed!");
}

int main() {
  test_highlight_database_persistence();
  std::println(std::cout, "\n✅ All tests passed!");
  return 0;
}
