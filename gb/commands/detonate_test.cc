// SPDX-License-Identifier: Apache-2.0

/// \file detonate_test.cc
/// \brief Unit tests for detonate command

import commands;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Create mine ship (activated)
  TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
      .owned_by(1, 0)
      .named("Mine")
      .in_star_orbit(0, SystemCoordinates{100.0, 100.0})
      .with_destruct(10)
      .with_on(true)
      .with_size(10)
      .with_tech(10.0)
      .build();

  // Create target ship nearby
  TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
      .owned_by(2, 0)
      .named("Target")
      .in_star_orbit(0, SystemCoordinates{105.0, 105.0})
      .with_armor(10)
      .with_crew(10, 0)
      .with_size(20)
      .with_tech(10.0)
      .build();
}

void test_detonate_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create GameObj
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // Execute detonate command: detonate #1
  ctx.assert_dispatch_success(g, {"detonate", "#1"});

  std::println(std::cout, "Command output: {}", g.out.str());

  // Verify mine was detonated (destroyed)
  const auto* detonated_mine = ctx.em.peek_ship(1);

  // Mine should be destroyed after detonation
  if (detonated_mine) {
    test::expect_false(detonated_mine->alive());
  }

  // Target ship should be affected by the detonation
  const auto* affected_target = ctx.em.peek_ship(2);
  test::expect_ne(affected_target, nullptr);
  // Target should either be destroyed or damaged
  test::expect_true(!affected_target->alive() || affected_target->damage() > 0);

  std::println(std::cout,
               "✓ detonate command: Mine detonation persisted to database");

  ctx.verify_universe_invariants();
}

void test_detonate_role_rejection() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create Guest Race
  TestWorldBuilder(ctx).add_race("GuestMineLayer", 100.0, /*guest=*/true,
                                 player_t{3});

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 3, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  ctx.assert_dispatch_rejected(g, {"detonate", "#1"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  ctx.verify_universe_invariants();
}

void test_detonate_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"detonate"});
  test::expect_contains(g.out.str(), "Syntax: detonate <mine>");

  // 2. Ship is not a mine
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"detonate", "#2"});

  // 3. Mine is not activated (on = false)
  ctx.em.mutate_ship(1, [](Ship& s) { s.on() = false; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"detonate", "#1"});
  test::expect_contains(g.out.str(), "not activated");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_detonate_happy_path();
  test_detonate_role_rejection();
  test_detonate_domain_errors();

  std::println(std::cout, "✓ detonate_test passed!");
  return 0;
}
