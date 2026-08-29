// SPDX-License-Identifier: Apache-2.0

/// \file land_test.cc
/// \brief Unit tests for land command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Lander", 100.0, false, player_t{1})
      .add_star("LandingStar", 10)
      .add_planet(0, PlanetType::EARTH);

  // Create a ship that can land (shuttle)
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(1, 0)
      .named("TestShuttle")
      .in_planet_orbit(0, 0, 0.0, 0.0)
      .with_crew(2, 0)
      .with_fuel(20.0)
      .build();
}

// Test: Land ship on planet coordinates
void test_land_on_planet() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);
  g.set_shipno(1);

  // Land on planet coordinates (1 AP deducted via dynamic AP)
  ctx.assert_dispatch_success(g, {"land", "#1", "5,5"}, 1);
  test::expect_contains(g.out.str(), "landed on planet");

  const auto* s = ctx.em.peek_ship(1);
  test::expect_true(s != nullptr);
  // Ship should be docked and landed after landing
  test::expect_true(s->docked());
  test::expect_true(s->is_landed());
  test::expect_false(s->is_docked());
  test::expect_eq(s->land_coords(), Coordinates(5, 5));

  ctx.verify_universe_invariants();
}

// Test: Cannot land docked ship
void test_cannot_land_docked_ship() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // First land the ship so it is docked
  ctx.assert_dispatch_success(g, {"land", "#1", "5,5"}, 1);

  // Ship is already docked from first landing
  const auto* s_before = ctx.em.peek_ship(1);
  bool was_docked = s_before->docked();

  // Try to land again on different coordinates
  ctx.assert_dispatch_rejected(g, {"land", "#1", "3,3"});

  // Should still be at original location
  const auto* s_after = ctx.em.peek_ship(1);
  test::expect_eq(s_after->docked(), was_docked);

  ctx.verify_universe_invariants();
}

// Test: Create carrier and shuttle for friendly landing
void test_land_on_friendly_carrier() {
  TestContext ctx;
  setup_test_world(ctx);

  // Reset shuttle to undocked state with land_coords at 5,5
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.docked() = false;
    s.whatorbits() = ScopeLevel::LEVEL_PLAN;
    s.set_land_coords({5, 5});
  });

  // Create a carrier landed at (5, 5)
  TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
      .owned_by(1, 0)
      .named("TestCarrier")
      .landed_on(0, 0, Coordinates(5, 5))
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Now the shuttle (already at 5,5 landed) can land on carrier
  ctx.assert_dispatch_success(g, {"land", "#1", "#2"}, 0);
  test::expect_true(g.out.str().contains("landed on") ||
                    g.out.str().contains("loaded onto"));

  const auto* shuttle_after = ctx.em.peek_ship(1);
  test::expect_true(shuttle_after != nullptr);
  test::expect_true(shuttle_after->docked());

  ctx.verify_universe_invariants();
}

// Test: Insufficient AP rejection
void test_land_insufficient_ap() {
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

  ctx.assert_dispatch_rejected(g, {"land", "#1", "5,5"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

// Test: Domain validation errors
void test_land_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"land", "#1"});
  test::expect_contains(g.out.str(), "Syntax: land <ship> <#mothership | x,y>");

  // 2. Invalid coordinates format
  ctx.assert_dispatch_rejected(g, {"land", "#1", "bad_coords"});
  test::expect_contains(g.out.str(), "Invalid coordinates format");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_land_on_planet();
  test_cannot_land_docked_ship();
  test_land_on_friendly_carrier();
  test_land_insufficient_ap();
  test_land_domain_errors();

  std::println(std::cout, "✓ land_test passed!");
  return 0;
}
