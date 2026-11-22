// SPDX-License-Identifier: Apache-2.0

/// \file zoom_test.cc
/// \brief Unit tests for zoom command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  Race race{};
  race.Playernum = 1;
  race.name = "Zoomers";
  race.Guest = false;

  RaceRepository races(store);
  races.save(race);
}

void test_zoom_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_scope_star(1);

  // 1. Query current zoom
  ctx.assert_dispatch_success(g, {"zoom"});
  test::expect_contains(g.out.str(), "Zoom value");

  // 2. Set decimal zoom factor
  g.out.str("");
  ctx.assert_dispatch_success(g, {"zoom", "2.5"});
  test::expect_contains(g.out.str(), "Zoom value 2.5");
  test::expect_eq(g.zoom[0], 2.5);

  // 3. Set rational fraction zoom factor (1/2 = 0.5)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"zoom", "1/2"});
  test::expect_contains(g.out.str(), "Zoom value 0.5");
  test::expect_eq(g.zoom[0], 0.5);

  // 4. Zoom at universe level (affects index 1) and UiMode::JSON envelope
  g.set_scope_univ();
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"zoom", "3.0"});
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), R"(Zoom value 3)");
  test::expect_eq(g.zoom[1], 3.0);
  // 5. Boundary zoom factors [1e-6, 1e6] succeed
  g.set_ui_mode(UiMode::ASCII);
  g.set_scope_star(1);
  ctx.assert_dispatch_success(g, {"zoom", "1e-6"});
  test::expect_eq(g.zoom[0], 1e-6);
  ctx.assert_dispatch_success(g, {"zoom", "1e6"});
  test::expect_eq(g.zoom[0], 1e6);
}

void test_zoom_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_scope_star(1);
  g.zoom[0] = 2.5;

  // Division by zero denominator
  ctx.assert_dispatch_rejected(g, {"zoom", "5/0"});
  test::expect_contains(g.out.str(), "Illegal denominator value");
  test::expect_eq(g.zoom[0], 2.5);

  // Malformed, non-finite, zero, negative, subnormal, and out-of-range zooms
  for (std::string bad :
       {"/2", "1/2abc", "banana", "2.5abc", "0", "-1", "1e-300", "1e9", "nan",
        "inf", "0/5", "1e200/1e-200"}) {
    g.out.str("");
    ctx.assert_dispatch_rejected(g, {"zoom", bad});
    test::expect_contains(g.out.str(), "Bad zoom value.");
    test::expect_eq(g.zoom[0], 2.5);
  }

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"zoom", "2.5", "extra"});
  test::expect_contains(g.out.str(), "Syntax: zoom [<amount>]");
}

}  // namespace

int main() {
  test_zoom_happy_path();
  test_zoom_domain_errors();

  std::println(std::cout, "✓ zoom_test passed!");
  return 0;
}
