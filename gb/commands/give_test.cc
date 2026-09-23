// SPDX-License-Identifier: Apache-2.0

/// \file give_test.cc
/// \brief Test give command functionality, ship ownership transfer, and
/// validation rules.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void test_give_dispatch() {
  std::println(std::cout, "Test: give command dispatch and ship transfer");
  TestContext ctx;
  ctx.with_standard_universe();

  // Establish mutual alliance between Federation (1) and Klingons (2)
  ctx.em.mutate_race(1, [](Race& r) { r.declare_alliance_with(player_t{2}); });
  ctx.em.mutate_race(2, [](Race& r) { r.declare_alliance_with(player_t{1}); });

  const shipnum_t ship_plan = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                  .owned_by(1, 0)
                                  .in_planet_orbit(1, 1)
                                  .build();

  const shipnum_t ship_star = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                  .owned_by(1, 0)
                                  .in_star_orbit(1)
                                  .build();

  const shipnum_t ship_univ =
      TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE).owned_by(1, 0).build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Happy path: give ship in planet orbit to mutual ally
  ctx.assert_dispatch_success(
      g, {"give", "Klingons", std::format("#{}", ship_plan.value)});
  test::expect_contains(g.out.str(), "Owner changed.");

  // Verify ownership changed
  ctx.em.clear_cache();
  const auto* transferred = ctx.em.peek_ship(ship_plan);
  test::expect_ne(transferred, nullptr);
  test::expect_eq(transferred->owner(), 2);
  test::expect_eq(transferred->governor(), 0);

  const auto* planet_verify = ctx.em.peek_planet(1, 1);
  test::expect_ne(planet_verify, nullptr);
  test::expect_eq(planet_verify->info(player_t{2}).explored, 1);

  const auto* star_verify = ctx.em.peek_star(1);
  test::expect_ne(star_verify, nullptr);
  test::expect_true(star_verify->is_explored_by(player_t{2}));
  std::println(std::cout, "    ✓ Ship in planet orbit given to ally");

  // 2. Happy path: give ship in star orbit
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"give", "Klingons", std::format("#{}", ship_star.value)});
  test::expect_contains(g.out.str(), "Owner changed.");
  std::println(std::cout, "    ✓ Ship in star orbit given to ally");

  // 3. Happy path: give ship in universe space
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"give", "Klingons", std::format("#{}", ship_univ.value)});
  test::expect_contains(g.out.str(), "Owner changed.");
  std::println(std::cout, "    ✓ Ship in universe space given to ally");

  // 4. Insufficient AP in star system
  const shipnum_t ship_no_ap = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                   .owned_by(1, 0)
                                   .in_star_orbit(1)
                                   .build();
  ctx.em.mutate_star(1, [](Star& s) { s.AP(player_t{1}) = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", ship_no_ap.value)});
  test::expect_contains(g.out.str(),
                        "You don't have enough action points in that system.");

  // 5. Insufficient AP in universe
  const shipnum_t ship_no_uap =
      TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE).owned_by(1, 0).build();
  ctx.em.mutate_universe([](universe_struct& u) { u.AP[player_t{1}] = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", ship_no_uap.value)});
  test::expect_contains(g.out.str(),
                        "You don't have enough universe action points.");

  // Restore AP for subsequent tests
  ctx.em.mutate_star(1, [](Star& s) { s.AP(player_t{1}) = 100; });
  ctx.em.mutate_universe([](universe_struct& u) { u.AP[player_t{1}] = 100; });

  // 6. Non-existent recipient
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"give", "NonExistent", "#1"});
  test::expect_contains(g.out.str(), "No such player.");

  // 7. Recipient is a guest
  ctx.em.mutate_race(2, [](Race& r) { r.Guest = true; });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", ship_no_ap.value)});
  test::expect_contains(g.out.str(), "You can't give this player anything.");
  ctx.em.mutate_race(2, [](Race& r) { r.Guest = false; });

  // 8. Not mutually allied
  ctx.em.mutate_race(2, [](Race& r) { r.rescind_alliance_with(player_t{1}); });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", ship_no_ap.value)});
  test::expect_contains(g.out.str(), "You two are not mutually allied.");
  ctx.em.mutate_race(2, [](Race& r) { r.declare_alliance_with(player_t{1}); });

  // 9. Illegal ship number format
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"give", "Klingons", "bad_ship"});
  test::expect_contains(g.out.str(), "Illegal ship number.");

  // 10. Missing ship in database
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"give", "Klingons", "#9999"});
  test::expect_contains(g.out.str(), "No such ship.");

  // 11. Ship not owned by donor
  const shipnum_t ship_p2 = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                .owned_by(2, 0)
                                .in_star_orbit(1)
                                .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", ship_p2.value)});

  // 12. Spore pod rejection
  const shipnum_t pod_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                               .owned_by(1, 0)
                               .in_star_orbit(1)
                               .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", pod_id.value)});
  test::expect_contains(g.out.str(),
                        "You cannot change the ownership of spore pods.");

  // 13. Crewed ship cannot be given away
  const shipnum_t crewed_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PROBE)
                                  .owned_by(1, 0)
                                  .in_star_orbit(1)
                                  .with_crew(10, 0)
                                  .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", crewed_id.value)});
  test::expect_contains(g.out.str(), "crew/mil on board");

  // 14. Carrier with loaded ships
  const shipnum_t carrier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                                   .owned_by(1, 0)
                                   .in_star_orbit(1)
                                   .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(1, 0)
      .docked_to(carrier_id, 1)
      .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"give", "Klingons", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "loaded on it");

  // 15. Command matrix validation (roles, guests, governor, scopes)
  TestCommandMatrix(ctx, "give")
      .with_valid_argv(
          {"give", "Klingons", std::format("#{}", ship_no_ap.value)})
      .with_invalid_argv({"give", "NonExistent", "#1"})
      .run_matrix(g);
}

}  // namespace

int main() {
  test_give_dispatch();
  std::println(std::cout, "\n✅ All give command tests passed!");
  return 0;
}
