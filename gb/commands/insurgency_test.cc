// SPDX-License-Identifier: Apache-2.0

/// \file insurgency_test.cc
/// \brief Unit tests for insurgency command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  ctx.em.mutate_race(1, [](Race& r) {
    r.leader().money = 1000000;
    r.morale = 100;
    r.fighters = 10;
  });

  ctx.em.mutate_race(2, [](Race& r) {
    r.morale = 50;
    r.fighters = 0;
  });

  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(player_t{2}).popn = 100;
    p.info(player_t{2}).troops = 0;
    p.info(player_t{2}).numsectsowned = 5;
    p.info(player_t{2}).tax = 100;
  });

  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    for (int i = 0; i < 5; i++) {
      auto& s = smap.get(Coordinates{i, 0});
      s.set_owner(2);
      s.set_popn_exact(200);
      s.set_troops(0);
      s.set_condition(SectorType::SEC_MOUNT);
    }
  });
}

void test_insurgency_happy_path_success() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_success(g, {"insurgency", "2", "500000"}, 10);
  test::expect_contains(g.out.str(), "Success!  You liberate");

  // Verify race money decreased
  ctx.em.clear_cache();
  const auto* saved_race = ctx.em.peek_race(1);
  test::expect_ne(saved_race, nullptr);
  test::expect_eq(saved_race->leader().money, 500000);

  // Verify planet tax rate inherited
  const auto* saved_planet = ctx.em.peek_planet(1, 1);
  test::expect_ne(saved_planet, nullptr);
  test::expect_eq(saved_planet->info(player_t{1}).tax, 100);
  std::println(std::cout, "    ✓ Insurgency success path verified");
}

void test_insurgency_failed_revolt() {
  TestContext ctx;
  setup_test_world(ctx);

  // Invert morale and troop balance so success chance drops to 0
  ctx.em.mutate_race(1, [](Race& r) {
    r.morale = 0;
    r.fighters = 0;
  });
  ctx.em.mutate_race(2, [](Race& r) {
    r.morale = 100;
    r.fighters = 20;
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(player_t{1}).popn = 1;
    p.info(player_t{1}).troops = 0;
    p.info(player_t{2}).troops = 1000;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_success(g, {"insurgency", "2", "100"}, 10);
  test::expect_contains(g.out.str(), "The insurgency failed!");
  std::println(std::cout, "    ✓ Insurgency failure path verified");
}

void test_insurgency_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set AP to 5 (< 10 required)
  ctx.em.mutate_star(1, [](Star& s) { s.AP(player_t{1}) = 5; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "5000"});
  test::expect_contains(g.out.str(), "action points");

  // Money must not have been deducted
  const auto* race = ctx.em.peek_race(1);
  test::expect_ne(race, nullptr);
  test::expect_eq(race->leader().money, 1000000);
  std::println(std::cout, "    ✓ Insufficient AP rejection verified");
}

void test_insurgency_role_and_scope_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Scope rejection (LEVEL_UNIV)
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "5000"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  // 2. Star control rejection (Star governed by Gov 1, tested by Gov 2)
  ctx.em.mutate_race(1, [](Race& r) { r.appoint_governor(2); });
  ctx.em.mutate_star(1, [](Star& s) {
    s.governor(player_t{1}) = 1;  // Star governed by Gov 1
  });
  g.out.str("");
  ctx.setup_game_obj(g, 1, 2);  // Player 1, Gov 2
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);
  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "5000"});
  test::expect_contains(g.out.str(), "not authorized");
  std::println(std::cout, "    ✓ Role and scope rejections verified");
}

void test_insurgency_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"insurgency", "2"});
  test::expect_contains(g.out.str(), "Syntax: insurgency <race> <money>");

  // 2. Non-existent player
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "99", "5000"});
  test::expect_contains(g.out.str(), "No such player.");

  // 3. Guest recipient rejection
  ctx.em.mutate_race(2, [](Race& r) { r.Guest = true; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "5000"});
  test::expect_contains(g.out.str(), "Don't be such a dickweed.");
  ctx.em.mutate_race(2, [](Race& r) { r.Guest = false; });

  // 4. Revolt against self
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "1", "5000"});
  test::expect_contains(g.out.str(), "yourself");

  // 5. Instigator has no population in star system
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(player_t{1}).popn = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "5000"});
  test::expect_contains(g.out.str(),
                        "You must have population in the star system");
  ctx.em.mutate_planet(1, 1,
                       [](Planet& p) { p.info(player_t{1}).popn = 1000; });

  // 6. Target player does not occupy this planet
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(player_t{2}).popn = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "5000"});
  test::expect_contains(g.out.str(), "does not occupy this planet");
  ctx.em.mutate_planet(1, 1,
                       [](Planet& p) { p.info(player_t{2}).popn = 1000; });

  // 7. Negative money amount
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "-100"});
  test::expect_contains(g.out.str(), "positive amount of money");

  // 8. Not enough money in treasury
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"insurgency", "2", "2000000"});
  test::expect_contains(g.out.str(), "Nice try");
  std::println(std::cout, "    ✓ Domain error rejections verified");

  // 9. Command matrix validation (roles, guests, governor, scopes)
  TestCommandMatrix(ctx, "insurgency")
      .with_valid_argv({"insurgency", "2", "100"})
      .with_invalid_argv({"insurgency", "99", "100"})
      .with_valid_scope(ScopeLevel::LEVEL_PLAN)
      .with_expected_star_ap(10)
      .run_matrix(g);
}

}  // namespace

int main() {
  test_insurgency_happy_path_success();
  test_insurgency_failed_revolt();
  test_insurgency_insufficient_ap();
  test_insurgency_role_and_scope_rejections();
  test_insurgency_domain_errors();

  std::println(std::cout, "\n✅ All insurgency tests passed!");
  return 0;
}
