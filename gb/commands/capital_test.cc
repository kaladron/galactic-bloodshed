// SPDX-License-Identifier: Apache-2.0

/// \file capital_test.cc
/// \brief Unit tests for capital command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_capital_matrix() {
  TestContext ctx;
  TestWorldBuilder(ctx)
      .add_race("TestRace", 100.0)
      .add_star("TestStar", 100, starnum_t{1})
      .add_planet(1, PlanetType::EARTH);

  // Landed government center ship
  shipnum_t landed_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                             .owned_by(1, 0)
                             .landed_on(1, 0, Coordinates{10, 10})
                             .build();

  // Orbiting non-landed government center ship
  shipnum_t orbit_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                            .owned_by(1, 0)
                            .in_star_orbit(1, 10.0, 10.0)
                            .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_snum(1);

  // 1. 4-Way Command Matrix runner on capital designation
  TestCommandMatrix(ctx, "capital")
      .with_valid_argv({"capital", std::to_string(landed_gov.value)})
      .with_invalid_argv({"capital", std::to_string(orbit_gov.value)})
      .with_valid_scope(ScopeLevel::LEVEL_UNIV)
      .with_expected_star_ap(50)
      .run_matrix(g);

  test::expect_eq(ctx.em.peek_race(1)->Gov_ship, landed_gov.value);
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 50);

  // 2. Query mode: Free inquiry (0 AP)
  ctx.assert_dispatch_success(g, {"capital"}, /*expected_star_ap_deducted=*/0);
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 50);

  // 3. Role Rejection: Governor 1 cannot designate capital
  ctx.setup_game_obj(g, 1, 1);
  g.set_snum(1);
  ctx.assert_dispatch_rejected(g,
                               {"capital", std::to_string(landed_gov.value)});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 0) may use this command.");
}

}  // namespace

int main() {
  test_capital_matrix();
  std::println(std::cout, "✓ capital_test passed!");
  return 0;
}
