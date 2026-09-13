// SPDX-License-Identifier: Apache-2.0

/// \file bless_test.cc
/// \brief Unit tests for bless command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void test_bless_matrix() {
  TestContext ctx;
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  TestCommandMatrix(ctx, "bless")
      .with_valid_argv({"bless", "2", "technology", "10"})
      .with_invalid_argv({"bless", "99", "technology", "10"})
      .with_valid_scope(ScopeLevel::LEVEL_PLAN)
      .with_invalid_scopes({ScopeLevel::LEVEL_STAR, ScopeLevel::LEVEL_UNIV,
                            ScopeLevel::LEVEL_SHIP})
      .with_expected_star_ap(0)
      .with_expected_univ_ap(0)
      .run_matrix(g);

  ctx.verify_universe_invariants();
}

void test_bless_role_and_scope_rejection() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Role Rejection: Mortal player 2 is rejected
  ctx.setup_game_obj(g, 2, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  g.set_god(false);
  ctx.assert_dispatch_rejected(g, {"bless", "2", "technology", "5"});
  test::expect_contains(g.out.str(), "Only deity can use this command.");

  // 2. Scope Rejection: Deity at LEVEL_UNIV scope is rejected
  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"bless", "2", "technology", "5"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  ctx.verify_universe_invariants();
}

void test_bless_race_characteristics() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Integer attributes
  ctx.assert_dispatch_success(g, {"bless", "2", "money", "500"});
  test::expect_eq(ctx.em.peek_race(2)->governor[0].money, 10500);

  ctx.assert_dispatch_success(g, {"bless", "2", "morale", "15"});
  test::expect_eq(ctx.em.peek_race(2)->morale, 15);

  ctx.assert_dispatch_success(g, {"bless", "2", "fertility", "75"});
  test::expect_eq(ctx.em.peek_race(2)->fertilize, 75);

  ctx.assert_dispatch_success(g, {"bless", "2", "IQ", "120"});
  test::expect_eq(ctx.em.peek_race(2)->IQ, 120);

  ctx.assert_dispatch_success(g, {"bless", "2", "fight", "80"});
  test::expect_eq(ctx.em.peek_race(2)->fighters, 80);

  ctx.assert_dispatch_success(g, {"bless", "2", "technology", "25"});
  test::expect_eq(ctx.em.peek_race(2)->tech, 125.0);

  ctx.assert_dispatch_success(g, {"bless", "2", "maxiq", "200"});
  test::expect_eq(ctx.em.peek_race(2)->IQ_limit, 200);

  // Float attributes
  ctx.assert_dispatch_success(g, {"bless", "2", "mass", "2.5"});
  test::expect_eq(ctx.em.peek_race(2)->mass, 2.5f);

  ctx.assert_dispatch_success(g, {"bless", "2", "metabolism", "1.75"});
  test::expect_eq(ctx.em.peek_race(2)->metabolism, 1.75f);

  ctx.assert_dispatch_success(g, {"bless", "2", "adventurism", "0.45"});
  test::expect_eq(ctx.em.peek_race(2)->adventurism, 0.45f);

  ctx.assert_dispatch_success(g, {"bless", "2", "birthrate", "1.25"});
  test::expect_eq(ctx.em.peek_race(2)->birthrate, 1.25f);

  // Flags & password
  ctx.assert_dispatch_success(g, {"bless", "2", "password", "secret42"});
  test::expect_eq(ctx.em.peek_race(2)->password, "secret42");

  ctx.assert_dispatch_success(g, {"bless", "2", "pods", "0"});
  test::expect_true(ctx.em.peek_race(2)->pods);
  ctx.assert_dispatch_success(g, {"bless", "2", "nopods", "0"});
  test::expect_false(ctx.em.peek_race(2)->pods);

  ctx.assert_dispatch_success(g, {"bless", "2", "collectiveiq", "0"});
  test::expect_true(ctx.em.peek_race(2)->collective_iq);
  ctx.assert_dispatch_success(g, {"bless", "2", "nocollectiveiq", "0"});
  test::expect_false(ctx.em.peek_race(2)->collective_iq);

  ctx.assert_dispatch_success(g, {"bless", "2", "guest", "0"});
  test::expect_true(ctx.em.peek_race(2)->Guest);

  ctx.assert_dispatch_success(g, {"bless", "2", "god", "0"});
  test::expect_true(ctx.em.peek_race(2)->God);

  ctx.assert_dispatch_success(g, {"bless", "2", "mortal", "0"});
  test::expect_false(ctx.em.peek_race(2)->God);
  test::expect_false(ctx.em.peek_race(2)->Guest);

  // Sector preferences
  auto expect_near = [](double actual, double expected, double eps = 1e-6) {
    test::expect_true(std::abs(actual - expected) < eps);
  };

  ctx.assert_dispatch_success(g, {"bless", "2", "water", "60"});
  expect_near(ctx.em.peek_race(2)->likes[SectorType::SEC_SEA], 0.60);

  ctx.assert_dispatch_success(g, {"bless", "2", "land", "70"});
  expect_near(ctx.em.peek_race(2)->likes[SectorType::SEC_LAND], 0.70);

  ctx.assert_dispatch_success(g, {"bless", "2", "mountain", "55"});
  expect_near(ctx.em.peek_race(2)->likes[SectorType::SEC_MOUNT], 0.55);

  ctx.assert_dispatch_success(g, {"bless", "2", "gas", "10"});
  expect_near(ctx.em.peek_race(2)->likes[SectorType::SEC_GAS], 0.10);

  ctx.assert_dispatch_success(g, {"bless", "2", "ice", "20"});
  expect_near(ctx.em.peek_race(2)->likes[SectorType::SEC_ICE], 0.20);

  ctx.assert_dispatch_success(g, {"bless", "2", "forest", "85"});
  expect_near(ctx.em.peek_race(2)->likes[SectorType::SEC_FOREST], 0.85);

  ctx.assert_dispatch_success(g, {"bless", "2", "desert", "30"});
  expect_near(ctx.em.peek_race(2)->likes[SectorType::SEC_DESERT], 0.30);

  ctx.assert_dispatch_success(g, {"bless", "2", "plated", "90"});
  expect_near(ctx.em.peek_race(2)->likes[SectorType::SEC_PLATED], 0.90);

  ctx.verify_universe_invariants();
}

void test_bless_planet_and_star() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // explorebit / noexplorebit
  ctx.assert_dispatch_success(g, {"bless", "2", "explorebit", "0"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).explored, 1);
  test::expect_true(ctx.em.peek_star(0)->explored()[2]);

  ctx.assert_dispatch_success(g, {"bless", "2", "noexplorebit", "0"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).explored, 0);

  // planetpopulation
  ctx.assert_dispatch_success(g, {"bless", "2", "planetpopulation", "2500"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).popn, 2500);

  // inhabited bit - verify latent bug fix (marks target race 2, not deity race
  // 1)
  ctx.assert_dispatch_success(g, {"bless", "2", "inhabited", "0"});
  test::expect_true(ctx.em.peek_star(0)->inhabited()[2]);

  // numsectsowned
  ctx.assert_dispatch_success(g, {"bless", "2", "numsectsowned", "42"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).numsectsowned, 42);

  ctx.verify_universe_invariants();
}

void test_bless_commodities() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // resources: full word and char
  ctx.assert_dispatch_success(g, {"bless", "2", "resource", "100"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).resource, 1100);
  ctx.assert_dispatch_success(g, {"bless", "2", "r", "50"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).resource, 1150);

  // destruct
  ctx.assert_dispatch_success(g, {"bless", "2", "destruct", "200"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).destruct, 1200);
  ctx.assert_dispatch_success(g, {"bless", "2", "d", "50"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).destruct, 1250);

  // fuel
  ctx.assert_dispatch_success(g, {"bless", "2", "fuel", "300"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).fuel, 1300);
  ctx.assert_dispatch_success(g, {"bless", "2", "f", "50"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).fuel, 1350);

  // crystals
  ctx.assert_dispatch_success(g, {"bless", "2", "crystal", "400"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).crystals, 400);
  ctx.assert_dispatch_success(g, {"bless", "2", "x", "50"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(2).crystals, 450);

  // action points
  ctx.assert_dispatch_success(g, {"bless", "2", "ap", "30"});
  test::expect_eq(ctx.em.peek_star(0)->AP(2), 130);
  ctx.assert_dispatch_success(g, {"bless", "2", "a", "20"});
  test::expect_eq(ctx.em.peek_star(0)->AP(2), 150);

  ctx.verify_universe_invariants();
}

void test_bless_error_handling() {
  TestContext ctx;
  ctx.with_standard_universe();
  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_god(true);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Invalid player number
  ctx.assert_dispatch_rejected(g, {"bless", "99", "technology", "10"});
  test::expect_contains(g.out.str(), "No such player number.");

  ctx.assert_dispatch_rejected(g, {"bless", "0", "technology", "10"});
  test::expect_contains(g.out.str(), "No such player number.");

  ctx.assert_dispatch_rejected(g, {"bless", "abc", "technology", "10"});
  test::expect_contains(g.out.str(), "No such player number.");

  // Unknown property or commodity
  ctx.assert_dispatch_rejected(g, {"bless", "2", "invalidprop", "10"});
  test::expect_contains(g.out.str(), "No such commodity.");

  // Invalid amounts
  ctx.assert_dispatch_rejected(g, {"bless", "2", "money", "bad"});
  test::expect_contains(g.out.str(), "Invalid amount.");

  ctx.assert_dispatch_rejected(g, {"bless", "2", "mass", "bad"});
  test::expect_contains(g.out.str(), "Invalid numeric value.");

  ctx.assert_dispatch_rejected(g, {"bless", "2", "water", "bad"});
  test::expect_contains(g.out.str(), "Invalid preference percentage.");

  ctx.assert_dispatch_rejected(g, {"bless", "2", "planetpopulation", "bad"});
  test::expect_contains(g.out.str(), "Invalid population count.");

  ctx.assert_dispatch_rejected(g, {"bless", "2", "numsectsowned", "bad"});
  test::expect_contains(g.out.str(), "Invalid sector count.");

  ctx.assert_dispatch_rejected(g, {"bless", "2", "fuel", "bad"});
  test::expect_contains(g.out.str(), "Invalid amount.");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_bless_matrix();
  test_bless_role_and_scope_rejection();
  test_bless_race_characteristics();
  test_bless_planet_and_star();
  test_bless_commodities();
  test_bless_error_handling();

  std::println(std::cout, "✓ bless_test passed!");
  return 0;
}
