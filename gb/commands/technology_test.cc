// SPDX-License-Identifier: Apache-2.0

/// \file technology_test.cc
/// \brief Unit tests for technology command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_technology_world(TestContext& ctx, ap_t star_ap = 10) {
  TestWorldBuilder(ctx)
      .add_race("TestRace")
      .add_star("TestStar", star_ap)
      .add_planet(1, PlanetType::EARTH, "TestPlanet");
  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.info(1).tech_invest = 100;
    planet.info(1).popn = 1000;
  });
}

// Test querying and setting planetary technology investment successfully
void test_technology_happy_paths() {
  TestContext ctx;
  setup_technology_world(ctx, 10);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Query current technology investment (costs 1 AP)
  ctx.assert_dispatch_success(g, {"technology"}, 1);
  test::expect_contains(g.out.str(), "Current investment : 100");
  test::expect_contains(g.out.str(), "Technology production/update:");

  // 2. Set technology investment to 500 (costs 1 AP)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"technology", "500"}, 1);
  test::expect_contains(g.out.str(), "New (ideal) tech production:");
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(1).tech_invest, 500);

  // 3. Set technology investment to 0
  g.out.str("");
  ctx.assert_dispatch_success(g, {"technology", "0"}, 1);
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(1).tech_invest, 0);
}

// Test technology command with insufficient AP
void test_technology_insufficient_ap() {
  TestContext ctx;
  setup_technology_world(ctx, 0);  // 0 AP (needs 1)

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_rejected(g, {"technology", "500"});
  test::expect_contains(g.out.str(), "You don't have 1 action points there.");
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(1).tech_invest, 100);
}

// Test technology command role and scope rejections
void test_technology_role_and_scope_rejections() {
  TestContext ctx;
  setup_technology_world(ctx, 10);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Star control rejection (Governor 2 on star assigned to Governor 1)
  ctx.setup_game_obj(g, 1, 2);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  ctx.assert_dispatch_rejected(g, {"technology", "200"});
  test::expect_contains(g.out.str(),
                        "You are not authorized to do that in this system.");

  // 2. Scope rejection (ScopeLevel::LEVEL_UNIV)
  g.out.str("");
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"technology", "200"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");
}

// Test technology command domain logic errors
void test_technology_domain_errors() {
  TestContext ctx;
  setup_technology_world(ctx, 10);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Domain error: Illegal negative value (0 AP deducted, investment unchanged)
  ctx.assert_dispatch_rejected(g, {"technology", "-100"});
  test::expect_contains(g.out.str(), "Illegal value.");
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(1).tech_invest, 100);
}

}  // namespace

int main() {
  test_technology_happy_paths();
  test_technology_insufficient_ap();
  test_technology_role_and_scope_rejections();
  test_technology_domain_errors();

  std::println(std::cout, "✓ technology_test passed!");
  return 0;
}
