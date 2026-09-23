// SPDX-License-Identifier: Apache-2.0

/// \file bombard_test.cc
/// \brief Unit tests for bombard command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Configure target sector (5,5) on planet (1,1) for defender (player 2)
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    auto& sect = smap.get(Coordinates{5, 5});
    sect.set_condition(SectorType::SEC_LAND);
    sect.set_popn_exact(100);
    sect.set_owner(2);
    sect.set_troops(10);
  });
  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.info(player_t{2}).numsectsowned += 1;
    planet.popn() += 100;
    planet.troops() += 10;
  });

  // Create attacker ship in orbit with guns and ammo
  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
      .owned_by(1, 0)
      .named("Battleship")
      .in_planet_orbit(1, 1)
      .with_guns(guntype_t::LIGHT, 10)
      .with_destruct(100)
      .with_crew(10, 10)
      .with_fuel(1000.0)
      .build();
}

void test_bombard_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Execute bombard command on sector 5,5 with strength 10 (deducts 1 Star AP
  // dynamically)
  ctx.assert_dispatch_success(g, {"bombard", "#1", "5,5", "10"}, 1);

  // Verify ship and planet still exist in database (persisted via
  // EntityManager)
  const auto* ship = ctx.em.peek_ship(1);
  test::expect_true(ship != nullptr);
  test::expect_eq(ship->number(), 1);
  test::expect_lt(ship->destruct(), 100);  // Ammo consumed

  const auto* planet_after = ctx.em.peek_planet(1, 1);
  test::expect_true(planet_after != nullptr);

  // Verify sector map persisted and target was damaged
  const auto* smap_after = ctx.em.peek_sectormap(1, 1);
  test::expect_true(smap_after != nullptr);

  ctx.verify_universe_invariants();
}

void test_bombard_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "5,5", "10"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_bombard_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create Guest Race
  TestWorldBuilder(ctx).add_race("GuestAttacker", 100.0, /*guest=*/true,
                                 player_t{3});

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Guest race rejection
  ctx.setup_game_obj(g, 3, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "5,5", "10"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // 2. Scope rejection (LEVEL_UNIV is not allowed for bombard)
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "5,5", "10"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  ctx.verify_universe_invariants();
}

void test_bombard_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"bombard"});
  test::expect_contains(g.out.str(),
                        "Syntax: bombard <ship> [<x,y> [<strength>]]");

  // 2. Inactive ship
  ctx.em.mutate_ship(1, [](Ship& s) { s.active() = false; });
  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "5,5", "10"});
  test::expect_contains(g.out.str(), "inactive");

  ctx.verify_universe_invariants();
}

void test_bombard_preconditions_afv_and_retaliation() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  const ap_t ap_before = ctx.em.peek_star(1)->AP(1);

  // 1. Invalid sector format and out-of-bounds sector do NOT deduct Star AP
  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "bad_coords", "10"});
  test::expect_contains(g.out.str(), "Invalid sector format.");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), ap_before);

  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "99,99", "10"});
  test::expect_contains(g.out.str(), "Illegal sector.");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), ap_before);

  // 2. Non-numeric or zero strength does NOT deduct Star AP
  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "5,5", "abc"});
  test::expect_contains(g.out.str(), "No attack.");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), ap_before);

  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "5,5", "0"});
  test::expect_contains(g.out.str(), "No attack.");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), ap_before);

  // 3. Spaceborne AFV cannot bombard
  const auto afv_id = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                          .owned_by(1, 0)
                          .in_planet_orbit(1, 1)
                          .with_guns(guntype_t::LIGHT, 5)
                          .with_destruct(20)
                          .with_crew(5, 5)
                          .build();
  ctx.assert_dispatch_rejected(
      g, {"bombard", std::format("#{}", afv_id.value), "5,5"});
  test::expect_contains(g.out.str(), "This ship is not landed on the planet.");

  // 4. Landed AFV must be adjacent to target sector
  ctx.em.mutate_ship(afv_id, [](Ship& s) {
    s.land_on_planet();
    s.set_land_coords({0, 0});
  });
  ctx.assert_dispatch_rejected(
      g, {"bombard", std::format("#{}", afv_id.value), "5,5"});
  test::expect_contains(g.out.str(), "You are not adjacent to that sector.");

  // 5. Planetary defense network blocks orbital bombardment without AP loss
  const auto pdef_id = TestShipBuilder(ctx.em, ShipType::OTYPE_PLANDEF)
                           .owned_by(2, 0)
                           .landed_on(1, 1, {5, 5})
                           .with_guns(guntype_t::MEDIUM, 5)
                           .with_destruct(20)
                           .with_crew(5, 5)
                           .build();
  ctx.assert_dispatch_rejected(g, {"bombard", "#1", "5,5", "10"});
  test::expect_contains(g.out.str(), "Target has planetary defense networks.");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), ap_before);

  // Remove planetary defense network
  ctx.em.mutate_ship(pdef_id, [](Ship& s) { s.alive() = false; });

  // 6. Laser bombardment + planetary defense gun retaliation + orbital
  // protector ship retaliation + random sector selection
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.laser() = true;
    s.fire_laser() = 25;
    s.mounted() = true;
  });
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    for (int y = 0; y < 2; ++y) {
      for (int x = 0; x < 10; ++x) {
        auto& s = smap.get({x, y});
        s.set_owner(2);
        s.set_popn_exact(1000);
        s.set_mobilization(100);
      }
    }
    auto& target = smap.get({5, 5});
    target.set_condition(SectorType::SEC_WASTED);
    target.set_owner(2);
    target.set_popn_exact(50000);
    target.set_troops(50000);
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(2).guns = 3;
    p.info(2).destruct = 5;
  });
  const auto protector_id = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                                .owned_by(2, 0)
                                .in_planet_orbit(1, 1)
                                .with_guns(guntype_t::LIGHT, 5)
                                .with_destruct(50)
                                .with_crew(10, 10)
                                .with_fuel(500.0)
                                .build();
  ctx.em.mutate_ship(protector_id, [](Ship& s) { s.protect().planet = true; });

  // Bombard with excessive strength (clamped to maxstrength) on (5,5)
  ctx.assert_dispatch_success(g, {"bombard", "#1", "5,5", "999"}, 1);
  test::expect_contains(g.out.str(), "Laser strength set to");
  test::expect_lt(ctx.em.peek_planet(1, 1)->info(2).destruct, 5, g.out.str());
  test::expect_lt(ctx.em.peek_ship(protector_id)->destruct(), 50);

  // Bombard with omitted coordinates (random sector selection)
  ctx.assert_dispatch_success(g, {"bombard", "#1"}, 1);
}

}  // namespace

int main() {
  test_bombard_happy_paths();
  test_bombard_insufficient_ap();
  test_bombard_role_and_scope_rejections();
  test_bombard_domain_errors();
  test_bombard_preconditions_afv_and_retaliation();

  std::println(std::cout, "✓ bombard_test passed!");
  return 0;
}
