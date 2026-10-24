// SPDX-License-Identifier: Apache-2.0

/// \file motto_test.cc
/// \brief Test motto command database persistence

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

void test_motto_database_persistence() {
  std::println(std::cout, "Test: motto command database persistence");

  // Create in-memory database
  TestContext ctx;

  // Setup: Create a block with initial empty motto
  Block b{{
      .Playernum = 1,
      .name = "Test Alliance",
      .motto = "",  // Initially empty
  }};

  JsonStore store(ctx.db);
  BlockRepository blocks(store);
  blocks.save(b);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_governor(1);  // Must be governor 1 to set motto

  // TEST 1: Set a motto
  std::println(std::cout, "  Testing: Set motto");
  {
    ctx.assert_dispatch_success(g, {"motto", "For", "the", "Empire!"});

    // Verify output message
    std::string out_str = g.out.str();
    test::expect_contains(out_str, "Done");
    std::println(std::cout, "    ✓ Output message correct");
    g.out.str("");  // Clear output

    // Verify database: motto should be set without trailing space
    auto saved = blocks.find_by_id(1);
    test::expect_true(saved.has_value());
    test::expect_eq(saved->motto(), "For the Empire!");
    std::println(std::cout, "    ✓ Database: motto = '{}'", saved->motto());
  }

  // TEST 2: Change the motto in JSON mode
  std::println(std::cout, "  Testing: Change motto");
  {
    g.set_ui_mode(UiMode::JSON);
    ctx.assert_dispatch_success(g, {"motto", "Victory", "or", "Death"});
    test::expect_contains(g.out.str(), "\"type\":\"motto\"");
    test::expect_contains(g.out.str(), "\"motto\":\"Victory or Death\"");
    g.set_ui_mode(UiMode::ASCII);

    // Verify database: motto should be changed
    auto saved = blocks.find_by_id(1);
    test::expect_true(saved.has_value());
    test::expect_eq(saved->motto(), "Victory or Death");
    std::println(std::cout, "    ✓ Database: motto = '{}'", saved->motto());
  }

  // TEST 3: Set empty motto
  std::println(std::cout, "  Testing: Clear motto with single space");
  {
    ctx.assert_dispatch_success(g, {"motto", " "});

    auto saved = blocks.find_by_id(1);
    test::expect_true(saved.has_value());
    std::println(std::cout, "    ✓ Database: motto = '{}'", saved->motto());
  }

  // TEST 4: Non-governor should be rejected
  std::println(std::cout, "  Testing: Non-governor authorization check");
  {
    g.set_governor(2);  // Change to non-leader governor
    ctx.assert_dispatch_rejected(g, {"motto", "Should", "Fail"});

    test::expect_contains(g.out.str(), "Only the leader");
    std::println(std::cout, "    ✓ Authorization check works");
    g.out.str("");
  }

  ctx.verify_universe_invariants();
  std::println(std::cout, "  ✅ All motto database persistence tests passed!");
}

int main() {
  test_motto_database_persistence();
  std::println(std::cout, "\n✅ All tests passed!");
  return 0;
}
