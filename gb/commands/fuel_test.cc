// SPDX-License-Identifier: Apache-2.0

/// \file fuel_test.cc
/// \brief Unit tests for fuel (proj_fuel) command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_fuel_matrix() {
  TestContext ctx;
  TestWorldBuilder(ctx)
      .add_race("Stargazers", 100.0)
      .add_star("OriginStar", 100, starnum_t{0})
      .add_planet(0, PlanetType::EARTH)
      .add_star("DestStar", 100, starnum_t{1})
      .add_planet(1, PlanetType::EARTH);

  // Position DestStar at (100, 100)
  ctx.em.mutate_star(1, [](Star& s) {
    s.xpos() = 100.0;
    s.ypos() = 100.0;
  });

  shipnum_t ship_num = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                           .owned_by(1, 0)
                           .named("Explorer")
                           .in_star_orbit(0, 0.0, 0.0)
                           .with_speed(2)
                           .with_fuel(100.0)
                           .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. 4-Way Command Matrix runner on fuel projection
  TestCommandMatrix(ctx, "fuel")
      .with_valid_argv(
          {"fuel", std::format("#{}", ship_num.value), "/DestStar"})
      .with_invalid_argv({"fuel", "#999", "/DestStar"})
      .with_valid_scope(ScopeLevel::LEVEL_STAR)
      .with_expected_star_ap(0)
      .run_matrix(g);

  test::expect_contains(g.out.str(), "FUEL ESTIMATES");

  // 2. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"fuel"});
  test::expect_contains(g.out.str(), "Syntax: fuel <#ship> [<destination>]");

  // 3. Bad argument format (not starting with #)
  ctx.assert_dispatch_rejected(g, {"fuel", "1", "/DestStar"});
  test::expect_contains(g.out.str(), "Invalid first option");
}

}  // namespace

int main() {
  test_fuel_matrix();
  std::println(std::cout, "✓ fuel_test passed!");
  return 0;
}
