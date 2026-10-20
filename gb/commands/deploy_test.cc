// SPDX-License-Identifier: Apache-2.0

/// \file deploy_test.cc
/// \brief Unit tests for the deploy command.

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.fighters = 10; });

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.popn() = 1000;
    planet.troops() = 500;
    planet.info(player_t{1}).popn = 1000;
    planet.info(player_t{1}).troops = 500;
    planet.info(player_t{1}).numsectsowned = 2;
  });

  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{0, 0}).set_owner(0);
    smap.get(Coordinates{0, 0}).set_popn_exact(0);

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

void test_deploy_happy_paths_and_json() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_success(g, {"deploy", "5,5", "k", "200"});

  ctx.em.clear_cache();
  const auto* smap = ctx.em.peek_sectormap(1, 1);
  test::expect_true(smap != nullptr);
  test::expect_eq(smap->get(Coordinates{5, 5}).get_troops(), 300);
  test::expect_eq(smap->get(Coordinates{5, 6}).get_troops(), 200);

  g.out.str("");
  g.set_ui_mode(UiMode::JSON);
  ctx.assert_dispatch_success(g, {"deploy", "5,5", "k", "100"});
  test::expect_contains(g.out.str(), "\"type\":\"deploy\"");
  test::expect_contains(g.out.str(), "\"moved_people\":100");

  ctx.verify_universe_invariants();
}

void test_deploy_military_assault() {
  TestContext ctx;
  setup_test_world(ctx);

  ctx.em.mutate_race(1, [](Race& r) {
    r.fighters = 50.0;
    r.tech = 100.0;
    r.morale = 150;
    r.absorb = true;
    r.likes[SectorType::SEC_LAND] = 50;
  });

  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 1.0;
    r.tech = 10.0;
    r.morale = 20;
    r.absorb = false;
    r.likes[SectorType::SEC_LAND] = 10;
  });

  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get(Coordinates{5, 5}).set_owner(1);
    smap.get(Coordinates{5, 5}).set_popn_exact(500);
    smap.get(Coordinates{5, 5}).set_troops(500);

    smap.get(Coordinates{5, 6}).set_owner(2);
    smap.get(Coordinates{5, 6}).set_popn_exact(10);
    smap.get(Coordinates{5, 6}).set_troops(10);
  });

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.popn() = 510;
    planet.troops() = 510;
    planet.info(player_t{1}).popn = 500;
    planet.info(player_t{1}).troops = 500;
    planet.info(player_t{1}).numsectsowned = 1;
    planet.info(player_t{2}).popn = 10;
    planet.info(player_t{2}).troops = 10;
    planet.info(player_t{2}).numsectsowned = 1;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_success(g, {"deploy", "5,5", "k", "200"});
  test::expect_contains(g.out.str(), "VICTORY");
  test::expect_contains(g.out.str(), "mil assault");

  ctx.em.clear_cache();
  const auto* smap = ctx.em.peek_sectormap(1, 1);
  test::expect_true(smap != nullptr);
  test::expect_eq(smap->get(Coordinates{5, 6}).get_owner(), 1);
  test::expect_ge(smap->get(Coordinates{5, 6}).get_troops(), 1);
  ctx.verify_universe_invariants();

  ctx.em.mutate_race(2, [](Race& r) {
    r.fighters = 100.0;
    r.tech = 200.0;
    r.morale = 200;
    r.absorb = false;
    r.likes[SectorType::SEC_LAND] = 100;
  });

  ctx.em.mutate_sectormap(1, 1, [](SectorMap& sm) {
    sm.get(Coordinates{5, 7}).set_owner(2);
    sm.get(Coordinates{5, 7}).set_popn_exact(1000);
    sm.get(Coordinates{5, 7}).set_troops(500);
  });

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.popn() += 1000;
    planet.troops() += 500;
    planet.info(player_t{2}).popn += 1000;
    planet.info(player_t{2}).troops += 500;
    planet.info(player_t{2}).numsectsowned += 1;
  });

  g.out.str("");
  ctx.assert_dispatch_success(g, {"deploy", "5,6", "k", "10"});
  test::expect_contains(g.out.str(), "repulsed");

  TestCommandMatrix(ctx, "deploy")
      .with_valid_argv({"deploy", "5,5", "k", "10"})
      .with_invalid_argv({"deploy", "5,5"})
      .with_valid_scope(ScopeLevel::LEVEL_PLAN)
      .run_matrix(g);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_deploy_happy_paths_and_json();
  test_deploy_military_assault();

  std::println(std::cout, "✓ deploy_test passed!");
  return 0;
}
