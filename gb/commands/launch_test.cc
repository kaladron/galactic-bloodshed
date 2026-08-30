// SPDX-License-Identifier: Apache-2.0

/// \file launch_test.cc
/// \brief Unit tests for launch and undock commands

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("TestRace", 100.0, false, player_t{1})
      .add_star("TestStar", 100)
      .add_planet(0, PlanetType::EARTH);

  // Create test shuttle landed on the planet at (5, 5)
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(1, 0)
      .named("TestShuttle")
      .landed_on(0, 0, Coordinates(5, 5))
      .with_fuel(1000.0)
      .build();
}

void test_launch_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Launch landed ship from planet (costs 1 Star AP)
  ctx.assert_dispatch_success(g, {"launch", "#1"}, 1);
  test::expect_contains(g.out.str(), "launched from planet");

  // Verify ship is no longer docked and has fuel consumed
  const auto* launched_ship = ctx.em.peek_ship(1);
  test::expect_true(launched_ship != nullptr);
  test::expect_eq(launched_ship->docked(), 0);
  test::expect_eq(launched_ship->whatdest(), ScopeLevel::LEVEL_UNIV);
  test::expect_lt(launched_ship->fuel(), 1000.0);  // Fuel consumed

  // Verify planet is now explored
  const auto* explored_planet = ctx.em.peek_planet(0, 0);
  test::expect_true(explored_planet != nullptr);
  test::expect_eq(explored_planet->explored(), 1);

  // 2. Undock alias dispatch
  // Re-dock ship to another ship to test undock
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.docked() = 1;
    s.whatdest() = ScopeLevel::LEVEL_SHIP;
    s.destshipno() = 1;  // Mock target
  });
  ctx.assert_dispatch_success(g, {"undock", "#1"}, 0);
  test::expect_contains(g.out.str(), "undocked");

  ctx.verify_universe_invariants();
}

void test_launch_insufficient_ap() {
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

  ctx.assert_dispatch_rejected(g, {"launch", "#1"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_launch_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"launch"});
  test::expect_contains(g.out.str(), "Syntax: launch <ship>");

  // 2. Launch non-docked/non-landed ship
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.docked() = 0;
    s.whatorbits() = ScopeLevel::LEVEL_PLAN;
    s.whatdest() = ScopeLevel::LEVEL_UNIV;
  });
  ctx.assert_dispatch_rejected(g, {"launch", "#1"});
  test::expect_contains(g.out.str(), "is not landed or docked");

  ctx.verify_universe_invariants();
}

void test_launch_canister_ships() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Create test canister ship landed on planet with initial count 5
  ship_struct canist_data{};
  canist_data.number = 2;
  canist_data.owner = 1;
  canist_data.governor = 0;
  canist_data.alive = true;
  canist_data.active = true;
  canist_data.type = ShipType::OTYPE_CANIST;
  canist_data.name = "DustCanister";
  canist_data.max_speed = 1;
  canist_data.whatorbits = ScopeLevel::LEVEL_PLAN;
  canist_data.storbits = starnum_t{0};
  canist_data.pnumorbits = planetnum_t{0};
  canist_data.whatdest = ScopeLevel::LEVEL_PLAN;
  canist_data.land_coords = {5, 5};
  canist_data.docked = 1;
  canist_data.fuel = 1000.0;
  canist_data.special = TimerData{.count = 5};
  auto canist_handle = ctx.em.create_ship(canist_data);
  const auto canist_id = canist_handle->number();

  ctx.assert_dispatch_success(
      g, {"launch", std::format("#{}", canist_id.value)}, 1);
  test::expect_contains(g.out.str(), "A cloud of dust envelopes your planet");

  ctx.em.clear_cache();
  const auto* launched = ctx.em.peek_ship(canist_id)->as<CanisterShip>();
  test::expect_ne(launched, nullptr);
  test::expect_eq(launched->count(), 0U);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_launch_happy_paths();
  test_launch_canister_ships();
  test_launch_insufficient_ap();
  test_launch_domain_errors();

  std::println(std::cout, "✓ launch_test passed!");
  return 0;
}
