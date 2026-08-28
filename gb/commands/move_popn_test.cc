// SPDX-License-Identifier: Apache-2.0

/// \file move_popn_test.cc
/// \brief Unit tests for move and deploy commands

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Testers", 100.0, false, player_t{1})
      .add_star("Test Star", 100, starnum_t{0})
      .add_planet(0, PlanetType::EARTH);

  // Set race fighters
  ctx.em.mutate_race(1, [](Race& r) { r.fighters = 10; });

  // Setup sectormap and planet population
  ctx.em.mutate_planet(0, 0, [](Planet& planet) {
    planet.popn() = 1000;
    planet.info(player_t{1}).numsectsowned = 2;
  });

  ctx.em.mutate_sectormap(0, 0, [](SectorMap& smap) {
    smap.get(Coordinates{5, 5}).set_owner(1);
    smap.get(Coordinates{5, 5}).set_popn_exact(1000);
    smap.get(Coordinates{5, 5}).set_troops(500);
    smap.get(Coordinates{5, 5}).set_condition(SectorType::SEC_MOUNT);

    smap.get(Coordinates{5, 6}).set_owner(1);
    smap.get(Coordinates{5, 6}).set_popn_exact(0);
    smap.get(Coordinates{5, 6}).set_troops(0);
    smap.get(Coordinates{5, 6}).set_condition(SectorType::SEC_LAND);
  });
}

void test_move_popn_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Test move command - 'k' moves south (y+1)
  ctx.assert_dispatch_success(g, {"move", "5,5", "k", "500"});

  // Verify population moved
  ctx.em.clear_cache();
  const auto* saved_smap = ctx.em.peek_sectormap(0, 0);
  test::expect_true(saved_smap != nullptr);

  const auto& source_sect = saved_smap->get(Coordinates{5, 5});
  test::expect_eq(source_sect.get_popn(), 500);

  const auto& dest_sect = saved_smap->get(Coordinates{5, 6});
  test::expect_eq(dest_sect.get_popn(), 500);

  // 2. Test deploy command - deploy 200 troops
  ctx.assert_dispatch_success(g, {"deploy", "5,5", "k", "200"});

  ctx.em.clear_cache();
  const auto* smap2 = ctx.em.peek_sectormap(0, 0);
  test::expect_true(smap2 != nullptr);
  test::expect_eq(smap2->get(Coordinates{5, 5}).get_troops(), 300);
  test::expect_eq(smap2->get(Coordinates{5, 6}).get_troops(), 200);

  ctx.verify_universe_invariants();
}

void test_move_popn_insufficient_ap() {
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

  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "500"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_move_popn_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Scope rejection (LEVEL_UNIV is not allowed for move/deploy)
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "500"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  // 2. Star control rejection
  ctx.em.mutate_star(0, [](Star& s) {
    s.governor(1) = 2;  // Player 1, Star governed by Gov 2
  });
  ctx.setup_game_obj(g, 1, 1);  // Player 1, Gov 1
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "500"});
  test::expect_contains(g.out.str(), "not authorized");

  ctx.verify_universe_invariants();
}

void test_move_popn_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"move", "5,5"});
  test::expect_contains(g.out.str(),
                        "Syntax: move <from_sector> <path> [<amount>]");

  // 2. Origin coordinates illegal
  ctx.assert_dispatch_rejected(g, {"move", "99,99", "k"});
  test::expect_contains(g.out.str(), "illegal");

  // 3. Bad value - more people than available in sector
  ctx.assert_dispatch_rejected(g, {"move", "5,5", "k", "99999"});
  test::expect_contains(g.out.str(), "Bad value");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_move_popn_happy_paths();
  test_move_popn_insufficient_ap();
  test_move_popn_role_and_scope_rejections();
  test_move_popn_domain_errors();

  std::println(std::cout, "✓ move_popn_test passed!");
  return 0;
}
