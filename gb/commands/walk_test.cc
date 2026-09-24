// SPDX-License-Identifier: Apache-2.0

/// \file walk_test.cc
/// \brief Unit tests for walk command.

import dallib;
import gb.entities;
import gb.services;
import test;
import commands;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Set race likes
  {
    ctx.em.mutate_race(1, [](Race& r) {
      r.likes[SectorType::SEC_MOUNT] = 1.0;
      r.likes[SectorType::SEC_LAND] = 1.0;
    });
  }

  // Setup sectormap
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{5, 5}).set_owner(1);
    smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_MOUNT);
    smap.get(Coordinates{5, 6}).set_owner(1);
    smap.get(Coordinates{5, 6}).set_condition(SectorType::SEC_MOUNT);
  });

  // Create AFV ship landed at (5, 5)
  TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
      .owned_by(1, 1)
      .named("AFV")
      .landed_on(1, 1, Coordinates(5, 5))
      .with_crew(10, 0)
      .with_fuel(100.0)
      .build();
}

void test_walk_role_and_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Guest rejection
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = true; });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"walk", "1", "k"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // Restore non-guest race
  ctx.em.mutate_race(1, [](Race& r) { r.Guest = false; });
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(1);
  g.set_pnum(1);

  // 2. Invalid ship number & non-existent ship rejection
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"walk", "0", "k"});
  test::expect_contains(g.out.str(), "Bad ship number.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"walk", "999", "k"});
  test::expect_contains(g.out.str(), "No such ship.");

  // 3. Unowned ship rejection
  shipnum_t enemy_afv = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                            .owned_by(2, 1)
                            .landed_on(1, 1, Coordinates(4, 4))
                            .with_crew(10, 0)
                            .with_fuel(100.0)
                            .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"walk", std::format("{}", enemy_afv.value), "k"});
  test::expect_contains(g.out.str(), "You do not control this ship.");

  // 4. Non-AFV ship rejection
  shipnum_t pod_ship = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                           .owned_by(1, 1)
                           .landed_on(1, 1, Coordinates(4, 5))
                           .with_crew(5, 0)
                           .with_fuel(100.0)
                           .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"walk", std::format("{}", pod_ship.value), "k"});
  test::expect_contains(g.out.str(), "This ship doesn't walk!");

  // 5. Unlanded AFV rejection
  shipnum_t orbiting_afv = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                               .owned_by(1, 1)
                               .in_planet_orbit(1, 1)
                               .with_crew(10, 0)
                               .with_fuel(100.0)
                               .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"walk", std::format("{}", orbiting_afv.value), "k"});
  test::expect_contains(g.out.str(), "This ship is not landed on a planet.");

  // 6. Crewless AFV rejection
  shipnum_t crewless_afv = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                               .owned_by(1, 1)
                               .landed_on(1, 1, Coordinates(3, 3))
                               .with_crew(0, 0)
                               .with_fuel(100.0)
                               .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"walk", std::format("{}", crewless_afv.value), "k"});
  test::expect_contains(g.out.str(), "No crew.");

  // 7. Insufficient fuel rejection
  shipnum_t empty_fuel_afv = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                                 .owned_by(1, 1)
                                 .landed_on(1, 1, Coordinates(3, 4))
                                 .with_crew(10, 0)
                                 .with_fuel(0.0)
                                 .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"walk", std::format("{}", empty_fuel_afv.value), "k"});
  test::expect_contains(g.out.str(), "You don't have 1.0 fuel to move it.");

  // 8. Illegal move direction & disliked sector condition
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"walk", "1", "5"});
  test::expect_contains(g.out.str(), "Illegal move.");

  ctx.em.mutate_race(1, [](Race& r) { r.likes[SectorType::SEC_MOUNT] = 0.0; });
  ctx.setup_game_obj(g);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"walk", "1", "k"});
  test::expect_contains(g.out.str(),
                        "Your ships cannot walk into that sector type!");

  // 9. Insufficient Star AP rejection
  ctx.em.mutate_race(1, [](Race& r) { r.likes[SectorType::SEC_MOUNT] = 1.0; });
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });
  ctx.setup_game_obj(g);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"walk", "1", "k"});
  test::expect_contains(g.out.str(), "You don't have 1 action points there.");

  ctx.verify_universe_invariants();
}

void test_walk_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(1);
  g.set_pnum(1);

  // 3. Test walk command success - move south (k or '2')
  ctx.assert_dispatch_success(g, {"walk", "1", "k"});

  // Verify AFV moved and AP deducted
  ctx.em.clear_cache();
  const auto* saved_ship = ctx.em.peek_ship(1);
  test::expect_true(saved_ship != nullptr);
  test::expect_true(saved_ship->land_coords() == Coordinates(5, 6));
  test::expect_lt(saved_ship->fuel(), 100.0);

  const auto* saved_star = ctx.em.peek_star(1);
  test::expect_true(saved_star != nullptr);
  test::expect_eq(saved_star->AP(1), 99);  // 1 Star AP deducted

  ctx.verify_universe_invariants();
}

void test_walk_afv_and_sector_combat() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create an armed Player 1 AFV at (5, 5)
  shipnum_t armed_afv = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                            .owned_by(1, 1)
                            .named("HeavyAFV")
                            .landed_on(1, 1, Coordinates(5, 5))
                            .with_guns(guntype_t::LIGHT, 2)
                            .with_destruct(50)
                            .with_armor(10)
                            .with_crew(10, 0)
                            .with_fuel(100.0)
                            .build();

  // Place a hostile Player 2 AFV with 1 destruct and hostile civilians/troops
  // at (5, 6)
  shipnum_t enemy_afv = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                            .owned_by(2, 1)
                            .named("EnemyTank")
                            .landed_on(1, 1, Coordinates(5, 6))
                            .with_guns(guntype_t::LIGHT, 1)
                            .with_crew(5, 0)
                            .with_fuel(50.0)
                            .with_destruct(1)
                            .with_armor(0)
                            .build();

  ctx.em.mutate_planet_and_sectors(1, 1, [](Planet& p, SectorMap& smap) {
    auto& sect = smap.get(Coordinates{5, 6});
    sect.set_owner(2);
    sect.set_popn_exact(1);
    sect.set_troops(0);
    p.sync_demographics(smap);
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Execute walk into hostile sector (5, 6)
  ctx.assert_dispatch_success(
      g, {"walk", std::format("{}", armed_afv.value), "k"});
  test::expect_lt(ctx.em.peek_ship(enemy_afv)->destruct(), 1);
  test::expect_lt(ctx.em.peek_ship(armed_afv)->destruct(), 50);

  // Also test unarmed AFV walking onto a hostile populated sector
  shipnum_t unarmed_afv = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                              .owned_by(1, 1)
                              .landed_on(1, 1, Coordinates(2, 2))
                              .with_crew(10, 0)
                              .with_fuel(50.0)
                              .with_destruct(0)
                              .build();
  ctx.em.mutate_planet_and_sectors(1, 1, [](Planet& p, SectorMap& smap) {
    auto& sect = smap.get(Coordinates{2, 3});
    sect.set_owner(2);
    sect.set_condition(SectorType::SEC_MOUNT);
    sect.set_popn_exact(50);
    p.sync_demographics(smap);
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"walk", std::format("{}", unarmed_afv.value), "k"});
  test::expect_contains(g.out.str(), "You have nothing to attack with!");
  test::expect_eq(ctx.em.peek_ship(unarmed_afv)->land_coords(),
                  Coordinates(2, 2));

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_walk_role_and_domain_errors();
  test_walk_happy_path();
  test_walk_afv_and_sector_combat();

  std::println(std::cout, "✓ walk_test passed!");
  return 0;
}
