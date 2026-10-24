// SPDX-License-Identifier: Apache-2.0

/// \file toggle_test.cc
/// \brief Test toggle command database persistence

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

void test_toggle_database_persistence() {
  std::println(std::cout, "Test: toggle command database persistence");

  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  const auto& leader = ctx.em.peek_race(1)->leader();
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(leader.homesystem);
  g.set_pnum(leader.homeplanetnum);

  // TEST 1: Display all toggle settings (no argument)
  {
    ctx.assert_dispatch_success(g, {"toggle"});
    std::string out_str = g.out.str();
    test::expect_contains(out_str, "gag is OFF");
    test::expect_contains(out_str, "inverse is ON");
    test::expect_contains(out_str, "color is OFF");
    test::expect_contains(out_str, "double_digits is OFF");
    test::expect_contains(out_str, "geography is OFF");
    test::expect_contains(out_str, "autoload is OFF");
    test::expect_contains(out_str, "compatibility is OFF");
    test::expect_contains(out_str, "VISIBLE");
    g.out.str("");
  }

  // TEST 2: Toggle gag setting on and off
  {
    ctx.assert_dispatch_success(g, {"toggle", "gag"});
    test::expect_contains(g.out.str(), "gag is now on");
    test::expect_true(ctx.em.peek_race(1)->leader().toggle.gag);
    g.out.str("");

    ctx.assert_dispatch_success(g, {"toggle", "gag"});
    test::expect_contains(g.out.str(), "gag is now off");
    test::expect_false(ctx.em.peek_race(1)->leader().toggle.gag);
    g.out.str("");
  }

  // TEST 3: Toggle inverse, color/colour, double_digits, geography, autoload,
  // compatibility, and visible
  {
    ctx.assert_dispatch_success(g, {"toggle", "inverse"});
    test::expect_false(ctx.em.peek_race(1)->leader().toggle.inverse);
    g.out.str("");

    ctx.assert_dispatch_success(g, {"toggle", "geography"});
    test::expect_true(ctx.em.peek_race(1)->leader().toggle.geography);
    g.out.str("");

    ctx.assert_dispatch_success(g, {"toggle", "color"});
    test::expect_contains(g.out.str(), "color is now on");
    test::expect_true(ctx.em.peek_race(1)->leader().toggle.color);
    g.out.str("");

    // Verify planetary map renders ANSI terrain foreground color when enabled
    ctx.assert_dispatch_success(g, {"map"});
    test::expect_contains(g.out.str(), "\x1b[39m");
    g.out.str("");

    // Toggle colour alias turns color back off
    ctx.assert_dispatch_success(g, {"toggle", "colour"});
    test::expect_contains(g.out.str(), "color is now off");
    test::expect_false(ctx.em.peek_race(1)->leader().toggle.color);
    g.out.str("");

    ctx.assert_dispatch_success(g, {"toggle", "double_digits"});
    test::expect_true(ctx.em.peek_race(1)->leader().toggle.double_digits);
    g.out.str("");

    ctx.assert_dispatch_success(g, {"toggle", "autoload"});
    test::expect_true(ctx.em.peek_race(1)->leader().toggle.autoload);
    g.out.str("");

    ctx.assert_dispatch_success(g, {"toggle", "compatibility"});
    test::expect_true(ctx.em.peek_race(1)->leader().toggle.compat);
    g.out.str("");

    ctx.assert_dispatch_success(g, {"toggle", "visible"});
    test::expect_true(ctx.em.peek_race(1)->leader().toggle.invisible);
    g.out.str("");
  }

  // TEST 4: Reject invalid toggle option and extra arguments; verify JSON mode
  {
    ctx.assert_dispatch_rejected(g, {"toggle", "invalid_option"});
    test::expect_contains(g.out.str(), "No such option");
    ctx.assert_dispatch_rejected(g, {"toggle", "gag", "extra"});
    test::expect_contains(g.out.str(), "Syntax: toggle [<option>]");

    g.set_ui_mode(UiMode::JSON);
    ctx.assert_dispatch_success(g, {"toggle"});
    test::expect_contains(g.out.str(), "\"type\":\"toggle_status\"");
    ctx.assert_dispatch_success(g, {"toggle", "gag"});
    test::expect_contains(g.out.str(), "\"type\":\"toggle\"");
    test::expect_contains(g.out.str(), "\"enabled\":true");
  }

  ctx.verify_universe_invariants();
  std::println(std::cout, "  ✅ All toggle database persistence tests passed!");
}

int main() {
  test_toggle_database_persistence();
  std::println(std::cout, "\n✅ All tests passed!");
  return 0;
}
