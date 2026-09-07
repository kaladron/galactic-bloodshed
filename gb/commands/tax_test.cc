// SPDX-License-Identifier: Apache-2.0

/// \file tax_test.cc
/// \brief Unit tests for tax command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

// Test querying and setting planetary tax rate successfully
void test_tax_happy_paths() {
  TestContext ctx;
  ctx.with_standard_universe();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Query current tax rate
  ctx.assert_dispatch_success(g, {"tax"});
  test::expect_contains(g.out.str(), "Current tax rate: 10%");
  test::expect_contains(g.out.str(), "Target: 10%");

  // 2. Set new tax rate to 25%
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tax", "25"});
  test::expect_contains(g.out.str(), "Set.");
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(1).newtax, 25);

  // 3. Set new tax rate to 100% (max)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tax", "100"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(1).newtax, 100);

  // 4. Set new tax rate to 0% (min)
  g.out.str("");
  ctx.assert_dispatch_success(g, {"tax", "0"});
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(1).newtax, 0);

  ctx.verify_universe_invariants();
}

// Test tax command role and scope rejections
void test_tax_role_and_scope_rejections() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Make player 2 a guest race
  ctx.em.mutate_race(2, [](Race& r) {
    r.Guest = true;
    r.Gov_ship = 100;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Guest race rejection
  ctx.setup_game_obj(g, 2, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // 2. Star control rejection (Governor 2 on star assigned to Governor 1)
  ctx.em.mutate_star(0, [](Star& s) {
    s.governor(1) = 1;  // Star assigned to Governor 1
  });
  g.out.str("");
  ctx.setup_game_obj(g, 1, 2);  // Player 1, Governor 2
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(),
                        "You are not authorized to do that in this system.");

  // 3. Scope rejection (ScopeLevel::LEVEL_UNIV)
  g.out.str("");
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(), "Invalid scope for this command.");

  ctx.verify_universe_invariants();
}

void test_tax_domain_errors() {
  TestContext ctx;
  ctx.with_standard_universe();

  // Reset Gov_ship to 0 (no government center active)
  ctx.em.mutate_race(1, [](Race& r) { r.Gov_ship = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Domain error: No government center active
  ctx.assert_dispatch_rejected(g, {"tax", "20"});
  test::expect_contains(g.out.str(), "You have no government center active.");

  // 2. Domain error: Illegal value (>100 or <0)
  ctx.em.mutate_race(1, [](Race& r) { r.Gov_ship = 100; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"tax", "150"});
  test::expect_contains(g.out.str(), "Illegal value.");
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(1).newtax, 10);

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"tax", "-10"});
  test::expect_contains(g.out.str(), "Illegal value.");
  test::expect_eq(ctx.em.peek_planet(0, 0)->info(1).newtax, 10);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_tax_happy_paths();
  test_tax_role_and_scope_rejections();
  test_tax_domain_errors();

  std::println(std::cout, "✓ tax_test passed!");
  return 0;
}
