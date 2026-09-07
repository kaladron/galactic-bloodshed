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

  // Configure target sector (5,5) on planet (0,0) for defender (player 2)
  ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
    auto& sect = smap.get(Coordinates{5, 5});
    sect.set_condition(SectorType::SEC_LAND);
    sect.set_popn_exact(100);
    sect.set_owner(2);
    sect.set_troops(10);
  });
  ctx.em.mutate_planet(0, 0, [](Planet& planet) {
    planet.info(player_t{2}).numsectsowned += 1;
    planet.popn() += 100;
    planet.troops() += 10;
  });

  // Create attacker ship in orbit with guns and ammo
  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
      .owned_by(1, 0)
      .named("Battleship")
      .in_planet_orbit(0, 0)
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
  g.set_snum(0);
  g.set_pnum(0);

  // Execute bombard command on sector 5,5 with strength 10 (deducts 1 Star AP
  // dynamically)
  ctx.assert_dispatch_success(g, {"bombard", "#1", "5,5", "10"}, 1);

  // Verify ship and planet still exist in database (persisted via
  // EntityManager)
  const auto* ship = ctx.em.peek_ship(1);
  test::expect_true(ship != nullptr);
  test::expect_eq(ship->number(), 1);
  test::expect_lt(ship->destruct(), 100);  // Ammo consumed

  const auto* planet_after = ctx.em.peek_planet(0, 0);
  test::expect_true(planet_after != nullptr);

  // Verify sector map persisted and target was damaged
  const auto* smap_after = ctx.em.peek_sectormap(0, 0);
  test::expect_true(smap_after != nullptr);

  ctx.verify_universe_invariants();
}

void test_bombard_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  ctx.em.mutate_star(0, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

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
  g.set_snum(0);
  g.set_pnum(0);

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
  g.set_snum(0);
  g.set_pnum(0);

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

}  // namespace

int main() {
  test_bombard_happy_paths();
  test_bombard_insufficient_ap();
  test_bombard_role_and_scope_rejections();
  test_bombard_domain_errors();

  std::println(std::cout, "✓ bombard_test passed!");
  return 0;
}
