// SPDX-License-Identifier: Apache-2.0

/// \file center_test.cc
/// \brief Unit tests for center command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_center_matrix() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  TestCommandMatrix(ctx, "center")
      .with_valid_argv({"center", "/Sol"})
      .with_invalid_argv({"center", "/NonexistentStar"})
      .with_valid_scope(ScopeLevel::LEVEL_UNIV)
      .with_expected_star_ap(0)
      .run_matrix(g);
}

void test_center_happy_path() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Center on Sol at (0, 0)
  ctx.assert_dispatch_success(g, {"center", "/Sol"});
  test::expect_eq(g.universe_center(), UniverseCoordinates(0.0, 0.0));

  // 2. Center on Vega at (300, 400)
  ctx.assert_dispatch_success(g, {"center", "/Vega"});
  test::expect_eq(g.universe_center(), UniverseCoordinates(300.0, 400.0));

  // 3. Center on Antares at (-300, -400)
  ctx.assert_dispatch_success(g, {"center", "/Antares"});
  test::expect_eq(g.universe_center(), UniverseCoordinates(-300.0, -400.0));
}

void test_center_domain_errors() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"center"});
  test::expect_contains(g.out.str(), "Syntax: center <star>");

  // 2. Extra args check (> 2 args)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"center", "/Sol", "extra"});
  test::expect_contains(g.out.str(), "center: which star?");

  // 3. Ship scope rejection (Ship #100 is Player 1 Government Center)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"center", "#100"});
  test::expect_contains(g.out.str(), "CHEATER!!!");

  // 4. Universe scope rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"center", "/"});
  test::expect_contains(g.out.str(), "center: bad scope.");

  // 5. Non-existent star
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"center", "/NonexistentStar"});
  test::expect_contains(g.out.str(), "center: bad scope.");
}

}  // namespace

int main() {
  test_center_matrix();
  test_center_happy_path();
  test_center_domain_errors();

  std::println(std::cout, "✓ center_test passed!");
  return 0;
}
