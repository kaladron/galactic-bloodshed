// SPDX-License-Identifier: Apache-2.0

/// \file defend_test.cc
/// \brief Unit tests for defend command

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Testers", 100.0, false, player_t{1})
      .add_race("Enemies", 100.0, false, player_t{2})
      .add_star("Test Star", 100, starnum_t{0})
      .add_planet(0, PlanetType::EARTH);

  // Setup planet info and sectors
  {
    auto planet_handle = ctx.em.get_planet(0, 0);
    planet_handle->info(player_t{1}).numsectsowned = 1;
    planet_handle->info(player_t{1}).guns = 50;
    planet_handle->info(player_t{1}).destruct = 100;
    planet_handle->popn() = 1000;

    auto smap_handle = ctx.em.get_sectormap(0, 0);
    smap_handle->get(5, 5).set_owner(1);
    smap_handle->get(5, 5).set_popn_exact(1000);
    smap_handle->get(5, 5).set_troops(500);
    smap_handle->get(5, 5).set_condition(SectorType::SEC_MOUNT);
  }

  // Create attacking enemy ship in planet orbit
  TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
      .owned_by(2, 0)
      .named("Factory")
      .in_planet_orbit(0, 0, 0.0, 0.0)
      .with_armor(100)
      .build();
}

void test_defend_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_success(g, {"defend", "1", "5,5", "25"});

  ctx.em.clear_cache();
  const auto* saved_planet = ctx.em.peek_planet(0, 0);
  test::expect_true(saved_planet != nullptr);
  test::expect_lt(saved_planet->info(player_t{1}).destruct, 100);

  ctx.verify_universe_invariants();
}

void test_defend_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set AP to 0
  {
    auto star_handle = ctx.em.get_star(0);
    star_handle->AP(1) = 0;
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "25"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_defend_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Scope rejection (LEVEL_UNIV)
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "25"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  // 2. Star control rejection
  {
    auto star_handle = ctx.em.get_star(0);
    star_handle->governor(1) = 2;  // Star governed by Gov 2
  }
  ctx.setup_game_obj(g, 1, 1);  // Player 1, Gov 1
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  ctx.assert_dispatch_rejected(g, {"defend", "1", "5,5", "25"});
  test::expect_contains(g.out.str(), "not authorized");

  ctx.verify_universe_invariants();
}

void test_defend_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args (< 3 args)
  ctx.assert_dispatch_rejected(g, {"defend", "1"});
  test::expect_contains(g.out.str(),
                        "Syntax: defend <ship> <sector> [<strength>]");

  // 2. Bad ship number
  ctx.assert_dispatch_rejected(g, {"defend", "abc", "5,5"});
  test::expect_contains(g.out.str(), "Bad ship number");

  // 3. Bad sector format
  ctx.assert_dispatch_rejected(g, {"defend", "1", "bad_coords"});
  test::expect_contains(g.out.str(), "Bad format");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_defend_happy_path();
  test_defend_insufficient_ap();
  test_defend_role_and_scope_rejections();
  test_defend_domain_errors();

  std::println(std::cout, "✓ defend_test passed!");
  return 0;
}
