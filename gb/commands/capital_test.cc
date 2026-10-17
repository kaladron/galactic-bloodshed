// SPDX-License-Identifier: Apache-2.0

/// \file capital_test.cc
/// \brief Unit tests for capital command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_capital_matrix() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) { r.appoint_governor(2); });

  // Landed government center ship on Earth (1, 1)
  shipnum_t landed_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                             .owned_by(1, 1)
                             .landed_on(1, 1, Coordinates{10, 10})
                             .build();

  // Orbiting non-landed government center ship around Sol (1)
  shipnum_t orbit_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                            .owned_by(1, 1)
                            .in_star_orbit(1, SystemCoordinates{10.0, 10.0})
                            .build();

  // Landed non-gov ship (pod) owned by player 1
  shipnum_t landed_pod = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                             .owned_by(1, 1)
                             .landed_on(1, 1, Coordinates{10, 10})
                             .build();

  // Landed gov ship owned by player 2
  shipnum_t enemy_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                            .owned_by(2, 1)
                            .landed_on(1, 1, Coordinates{10, 10})
                            .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_snum(1);

  // 0. Error paths before designation
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"capital"});
  test::expect_contains(g.out.str(), "Change the capital to be what ship?");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"capital", "invalid"});
  test::expect_contains(g.out.str(), "Specify a valid ship number.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"capital", "999"});
  test::expect_contains(g.out.str(), "Change the capital to be what ship?");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"capital", std::to_string(enemy_gov.value)});
  test::expect_contains(g.out.str(), "You can't do that!");

  g.out.str("");
  ctx.assert_dispatch_rejected(g,
                               {"capital", std::to_string(landed_pod.value)});
  test::expect_contains(g.out.str(), "That ship is not a");

  // 1. 4-Way Command Matrix runner on capital designation
  TestCommandMatrix(ctx, "capital")
      .with_valid_argv({"capital", std::to_string(landed_gov.value)})
      .with_invalid_argv({"capital", std::to_string(orbit_gov.value)})
      .with_valid_scope(ScopeLevel::LEVEL_UNIV)
      .with_expected_star_ap(50)
      .run_matrix(g);

  test::expect_eq(ctx.em.peek_race(1)->Gov_ship, landed_gov);
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 50);

  // 2. Query mode: Free inquiry (0 AP) in ASCII and JSON
  g.out.str("");
  ctx.assert_dispatch_success(g, {"capital"}, /*expected_star_ap_deducted=*/0);
  test::expect_contains(g.out.str(), "Efficiency of governmental center:");
  test::expect_eq(ctx.em.peek_star(1)->AP(player_t{1}), 50);

  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"capital"}, /*expected_star_ap_deducted=*/0);
  test::expect_contains(g.out.str(), R"("type":"capital")");
  test::expect_contains(g.out.str(), R"("designated":false)");
  g.set_ui_mode(UiMode::ASCII);

  // 3. Role Rejection: Governor 2 cannot designate capital
  ctx.setup_game_obj(g, 1, 2);
  g.set_snum(1);
  ctx.assert_dispatch_rejected(g,
                               {"capital", std::to_string(landed_gov.value)});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 1) may use this command.");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_capital_matrix();
  std::println(std::cout, "✓ capital_test passed!");
  return 0;
}
