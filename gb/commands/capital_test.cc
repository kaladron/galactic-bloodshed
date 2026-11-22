// SPDX-License-Identifier: Apache-2.0

/// \file capital_test.cc
/// \brief Unit tests for capital command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

constexpr player_t p1 = 2;
constexpr player_t p2 = 3;

void test_capital_matrix() {
  TestContext ctx;
  ctx.with_galaxy();
  TestWorldBuilder world(ctx);
  world.add_race("Federation", 100.0, false, p1);
  world.add_race("Klingons", 100.0, false, p2);

  ctx.em.mutate_race(p1, [](Race& r) { r.appoint_governor(2); });
  ctx.em.mutate_star(1, [](Star& s) {
    s.mark_explored_by(p1);
    s.mark_inhabited_by(p1);
    s.AP(p1) = 100;
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.mark_explored_by(p1); });

  // Landed government center ship on Earth (1, 1)
  shipnum_t landed_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                             .owned_by(p1, 1)
                             .landed_on(1, 1, Coordinates{5, 5})
                             .build();

  // Orbiting non-landed government center ship around Sol (1)
  shipnum_t orbit_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                            .owned_by(p1, 1)
                            .in_star_orbit(1, SystemCoordinates{10.0, 10.0})
                            .build();

  // Landed non-gov ship (pod) owned by p1
  shipnum_t landed_pod = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                             .owned_by(p1, 1)
                             .landed_on(1, 1, Coordinates{5, 5})
                             .build();

  // Landed gov ship owned by p2
  shipnum_t enemy_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                            .owned_by(p2, 1)
                            .landed_on(1, 1, Coordinates{5, 5})
                            .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, p1, 1);
  g.set_scope_star(1);

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
      .with_expected_star_ap(50)
      .run_matrix(g);

  test::expect_eq(ctx.em.peek_race(p1)->Gov_ship, landed_gov);
  test::expect_eq(ctx.em.peek_star(1)->AP(p1), 50);

  // 2. Query mode: Free inquiry (0 AP) in ASCII and JSON
  g.out.str("");
  ctx.assert_dispatch_success(g, {"capital"}, /*expected_star_ap_deducted=*/0);
  test::expect_contains(g.out.str(), "Efficiency of governmental center:");
  test::expect_eq(ctx.em.peek_star(1)->AP(p1), 50);

  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"capital"}, /*expected_star_ap_deducted=*/0);
  test::expect_contains(g.out.str(), R"("type":"message")");
  test::expect_contains(g.out.str(), "Efficiency of governmental center:");
  g.set_ui_mode(UiMode::ASCII);

  // 3. Role Rejection: Governor 2 cannot designate capital
  ctx.setup_game_obj(g, p1, 2);
  g.set_scope_star(1);
  ctx.assert_dispatch_rejected(g,
                               {"capital", std::to_string(landed_gov.value)});
  test::expect_contains(g.out.str(),
                        "Only the leader (Governor 1) may use this command.");

  // 4. Governmental Center berthed inside a Habitat in stellar orbit succeeds,
  // whereas one inside a deep-space Habitat is rejected
  ctx.setup_game_obj(g, p1, 1);
  g.set_scope_star(1);
  ctx.em.mutate_star(1, [](Star& s) { s.AP(p1) = 100; });
  const shipnum_t star_hab = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                                 .owned_by(p1, 1)
                                 .in_star_orbit(1)
                                 .with_max_hanger(100)
                                 .with_crew(50, 0)
                                 .build();
  const shipnum_t hab_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                                .owned_by(p1, 1)
                                .docked_to(star_hab, 1)
                                .with_crew(10, 0)
                                .build();
  ctx.assert_dispatch_success(g, {"capital", std::format("#{}", hab_gov.value)},
                              /*expected_star_ap_deducted=*/50);
  test::expect_eq(ctx.em.peek_race(p1)->Gov_ship, hab_gov);

  const shipnum_t deep_hab = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                                 .owned_by(p1, 1)
                                 .in_deep_space()
                                 .with_max_hanger(100)
                                 .with_crew(50, 0)
                                 .build();
  const shipnum_t deep_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                                 .owned_by(p1, 1)
                                 .in_deep_space()
                                 .with_crew(10, 0)
                                 .build();
  ctx.em.mutate_ship(deep_gov, [&](Ship& s) {
    s.dock_into_carrier(*ctx.em.peek_ship(deep_hab));
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(g,
                               {"capital", std::format("#{}", deep_gov.value)});
  test::expect_contains(g.out.str(), "Try landing this ship first!");

  // Non-Habitat carrier is rejected; Habitat in planetary orbit succeeds
  const shipnum_t non_hab = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                                .owned_by(p1, 1)
                                .in_planet_orbit(1, 1)
                                .with_max_hanger(100)
                                .with_crew(50, 0)
                                .build();
  const shipnum_t non_hab_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                                    .owned_by(p1, 1)
                                    .docked_to(non_hab, 1)
                                    .with_crew(10, 0)
                                    .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"capital", std::format("#{}", non_hab_gov.value)});
  test::expect_contains(g.out.str(), "Try landing this ship first!");

  ctx.em.mutate_star(1, [](Star& s) { s.AP(p1) = 100; });
  const shipnum_t plan_hab = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                                 .owned_by(p1, 1)
                                 .in_planet_orbit(1, 1)
                                 .with_max_hanger(100)
                                 .with_crew(50, 0)
                                 .build();
  const shipnum_t plan_gov = TestShipBuilder(ctx.em, ShipType::OTYPE_GOV)
                                 .owned_by(p1, 1)
                                 .docked_to(plan_hab, 1)
                                 .with_crew(10, 0)
                                 .build();
  ctx.assert_dispatch_success(
      g, {"capital", std::format("#{}", plan_gov.value)}, 50);
  test::expect_eq(ctx.em.peek_race(p1)->Gov_ship, plan_gov);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_capital_matrix();
  std::println(std::cout, "✓ capital_test passed!");
  return 0;
}
