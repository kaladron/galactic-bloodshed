// SPDX-License-Identifier: Apache-2.0

/// \file dock_test.cc
/// \brief Unit tests for dock and assault commands

import commands;
import dallib;
import gblib;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("NormalRace", 10.0, false, player_t{1})
      .add_race("GuestRace", 10.0, true, player_t{2})
      .add_star("TestStar", 10);

  // Ship 1: Player 1 Fighter
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(1, 0)
      .named("Docker")
      .in_star_orbit(0, 100.0, 200.0)
      .with_crew(0, 10)
      .with_fuel(100.0)
      .build();

  // Ship 2: Player 1 Carrier (close to ship 1)
  TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
      .owned_by(1, 0)
      .named("Carrier")
      .in_star_orbit(0, 100.0, 200.0)
      .with_fuel(100.0)
      .build();

  // Ship 3: Player 2 Cargo Ship (target for assault)
  TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
      .owned_by(2, 0)
      .named("Target")
      .in_star_orbit(0, 100.0, 200.0)
      .with_fuel(100.0)
      .build();

  // Ship 4: Far away ship
  TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
      .owned_by(1, 0)
      .named("FarTarget")
      .in_star_orbit(0, 500.0, 500.0)
      .with_fuel(100.0)
      .build();
}

void test_dock_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Successful dock (0 AP)
  ctx.assert_dispatch_success(g, {"dock", "#1", "#2"}, 0);
  test::expect_contains(g.out.str(), "docked with");

  const auto* s1 = ctx.em.peek_ship(1);
  const auto* s2 = ctx.em.peek_ship(2);
  test::expect_true(s1 != nullptr);
  test::expect_true(s2 != nullptr);
  test::expect_eq(s1->docked(), 1);
  test::expect_eq(s1->whatdest(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(s1->destshipno(), 2);
  test::expect_eq(s2->docked(), 1);
  test::expect_eq(s2->whatdest(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(s2->destshipno(), 1);

  // 2. Successful assault (1 AP deducted via dynamic AP)
  // Undock first for assault test
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.docked() = 0;
    s.destshipno() = 0;
    s.whatdest() = ScopeLevel::LEVEL_UNIV;
  });
  ctx.assert_dispatch_success(g, {"assault", "#1", "#3"}, 1);
  test::expect_true(g.out.str().contains("VICTORY") ||
                    g.out.str().contains("CAPTURED"));

  ctx.verify_universe_invariants();
}

void test_assault_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0 for Player 1
  ctx.em.mutate_star(0, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_assault_guest_rejection() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 2, 0);  // Player 2 is guest
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  ctx.assert_dispatch_rejected(g, {"assault", "#3", "#1"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  ctx.verify_universe_invariants();
}

void test_dock_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Min args violation (< 3 args)
  ctx.assert_dispatch_rejected(g, {"dock", "#1"});
  test::expect_contains(g.out.str(), "Syntax: dock <ship> <target_ship>");

  // 2. Docking with self
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#1"});
  test::expect_contains(g.out.str(), "You can't dock with yourself!");

  // 3. Out of range docking
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#4"});
  test::expect_contains(g.out.str(), "10.00 or closer");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_dock_happy_paths();
  test_assault_insufficient_ap();
  test_assault_guest_rejection();
  test_dock_domain_errors();

  std::println(std::cout, "✓ dock_test passed!");
  return 0;
}
