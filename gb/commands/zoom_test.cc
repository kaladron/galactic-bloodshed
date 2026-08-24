// SPDX-License-Identifier: Apache-2.0

/// \file zoom_test.cc
/// \brief Unit tests for zoom command

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  Race race{};
  race.Playernum = 1;
  race.name = "Zoomers";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);
}

void test_zoom_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);

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

  // 4. Zoom at universe level (affects index 1)
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"zoom", "3.0"});
  test::expect_contains(g.out.str(), "Zoom value 3");
  test::expect_eq(g.zoom[1], 3.0);
}

void test_zoom_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);

  // Division by zero denominator
  ctx.assert_dispatch_rejected(g, {"zoom", "5/0"});
  test::expect_contains(g.out.str(), "Illegal denominator value");
}

}  // namespace

int main() {
  test_zoom_happy_path();
  test_zoom_domain_errors();

  std::println(std::cout, "✓ zoom_test passed!");
  return 0;
}
