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
  ctx.with_standard_universe();

  // Create test shuttle landed on the planet at (5, 5)
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(1, 0)
      .named("TestShuttle")
      .landed_on(0, 0, Coordinates(5, 5))
      .with_fuel(20.0)
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
    s.dock_with_ship(1);  // Mock target
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
    s.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
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
  const auto canist_id = TestShipBuilder(ctx.em, ShipType::OTYPE_CANIST)
                             .owned_by(1)
                             .named("DustCanister")
                             .with_alive(true)
                             .with_active(true)
                             .with_max_speed(1)
                             .landed_on(0, 0, {5, 5})
                             .with_fuel(1000.0)
                             .with_special(TimerData{.count = 5})
                             .build();

  ctx.assert_dispatch_success(
      g, {"launch", std::format("#{}", canist_id.value)}, 1);
  test::expect_contains(g.out.str(), "A cloud of dust envelopes your planet");

  ctx.em.clear_cache();
  const auto* launched = ctx.em.peek_ship(canist_id)->as<CanisterShip>();
  test::expect_ne(launched, nullptr);
  test::expect_eq(launched->count(), 0U);

  // Also test Greenhouse gas canister launch message
  const auto green_id = TestShipBuilder(ctx.em, ShipType::OTYPE_GREEN)
                            .owned_by(1)
                            .named("GreenCanister")
                            .with_alive(true)
                            .with_active(true)
                            .with_max_speed(1)
                            .landed_on(0, 0, {5, 5})
                            .with_fuel(1000.0)
                            .with_special(TimerData{.count = 3})
                            .build();
  ctx.assert_dispatch_success(g, {"launch", std::format("#{}", green_id.value)},
                              1);
  test::expect_contains(g.out.str(), "Greenhouse gases surround the planet.");

  ctx.verify_universe_invariants();
}

void test_launch_from_carrier_all_scopes() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_SHIP);
  g.set_snum(0);
  g.set_pnum(0);

  const auto carrier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                              .owned_by(1, 0)
                              .named("MotherCarrier")
                              .landed_on(0, 0, {5, 5})
                              .with_max_hanger(100)
                              .build();

  // 1. Online factory berthed in carrier cannot be launched
  const auto fac_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                          .owned_by(1, 0)
                          .with_on(true)
                          .build();
  ctx.em.mutate_ship(fac_id, [&](Ship& f) { f.dock_into_carrier(carrier_id); });
  ctx.assert_dispatch_rejected(g, {"launch", std::format("#{}", fac_id.value)});
  test::expect_contains(g.out.str(),
                        "Factories cannot be launched once turned on.");

  // 2. Dock shuttle #1 into landed carrier and launch onto planet surface
  test::expect_true(ctx.em.dock_carrier(1, carrier_id).has_value());
  ctx.em.mutate_ship(1, [&](Ship& s) { s.dock_into_carrier(carrier_id); });
  ctx.assert_dispatch_success(g, {"launch", "#1"}, 0);
  test::expect_contains(g.out.str(), "Landed on Sol/Earth.");
  test::expect_true(ctx.em.peek_ship(1)->is_landed());

  // 3. Carrier in LEVEL_PLAN -> shuttle launches into LEVEL_PLAN
  ctx.em.mutate_ship(
      carrier_id, [](Ship& c) { c.launch_to_orbit(ScopeLevel::LEVEL_PLAN); });
  ctx.em.mutate_ship(1, [&](Ship& s) {
    s.dock_into_carrier(carrier_id);
    s.whatdest() = ScopeLevel::LEVEL_SHIP;
  });
  ctx.em.mutate_ship(carrier_id, [](Ship& c) {
    c.hanger() += 10;
    c.set_mass(c.mass() + 10.0);
  });
  ctx.assert_dispatch_success(g, {"launch", "#1"}, 0);
  test::expect_contains(g.out.str(), "Orbiting Sol/Earth.");
  test::expect_eq(ctx.em.peek_ship(1)->whatorbits(), ScopeLevel::LEVEL_PLAN);

  // 4. Carrier in LEVEL_STAR -> shuttle launches into LEVEL_STAR
  ctx.em.mutate_ship(
      carrier_id, [](Ship& c) { c.launch_to_orbit(ScopeLevel::LEVEL_STAR); });
  ctx.em.mutate_ship(1, [&](Ship& s) {
    s.dock_into_carrier(carrier_id);
    s.whatdest() = ScopeLevel::LEVEL_SHIP;
  });
  ctx.em.mutate_ship(carrier_id, [](Ship& c) {
    c.hanger() += 10;
    c.set_mass(c.mass() + 10.0);
  });
  ctx.assert_dispatch_success(g, {"launch", "#1"}, 0);
  test::expect_contains(g.out.str(), "Orbiting Sol.");
  test::expect_eq(ctx.em.peek_ship(1)->whatorbits(), ScopeLevel::LEVEL_STAR);

  // 5. Carrier in LEVEL_UNIV -> shuttle launches into LEVEL_UNIV
  ctx.em.mutate_ship(
      carrier_id, [](Ship& c) { c.launch_to_orbit(ScopeLevel::LEVEL_UNIV); });
  ctx.em.mutate_ship(1, [&](Ship& s) {
    s.dock_into_carrier(carrier_id);
    s.whatdest() = ScopeLevel::LEVEL_SHIP;
  });
  ctx.em.mutate_ship(carrier_id, [](Ship& c) {
    c.hanger() += 10;
    c.set_mass(c.mass() + 10.0);
  });
  ctx.assert_dispatch_success(g, {"launch", "#1"}, 0);
  test::expect_contains(g.out.str(), "Universe level.");
  test::expect_eq(ctx.em.peek_ship(1)->whatorbits(), ScopeLevel::LEVEL_UNIV);

  // 6. Nested carrier (s2.whatorbits() == LEVEL_SHIP) rejected
  const auto super_id = TestShipBuilder(ctx.em, ShipType::STYPE_HABITAT)
                            .owned_by(1, 0)
                            .in_star_orbit(0)
                            .build();
  ctx.em.mutate_ship(carrier_id,
                     [&](Ship& c) { c.dock_into_carrier(super_id); });
  ctx.em.mutate_ship(1, [&](Ship& s) { s.dock_into_carrier(carrier_id); });
  ctx.assert_dispatch_rejected(g, {"launch", "#1"});
  test::expect_contains(
      g.out.str(), "mothership is currently berthed inside another vessel");
}

void test_launch_planet_fuel_precheck_preserves_ap_and_coords() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  const auto no_fuel_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                              .owned_by(1, 0)
                              .landed_on(0, 0, {5, 5})
                              .with_fuel(0.0)
                              .build();
  ctx.em.mutate_ship(no_fuel_id,
                     [](Ship& s) { s.set_coordinates({42.0, 42.0}); });

  const ap_t ap_before = ctx.em.peek_star(0)->AP(1);
  ctx.assert_dispatch_rejected(
      g, {"launch", std::format("#{}", no_fuel_id.value)});
  test::expect_contains(g.out.str(), "does not have enough fuel");

  // Verify AP was NOT deducted and landed coordinates were NOT corrupted
  test::expect_eq(ctx.em.peek_star(0)->AP(1), ap_before);
  test::expect_eq(ctx.em.peek_ship(no_fuel_id)->coordinates(),
                  UniverseCoordinates{42.0, 42.0});

  // Also test overloaded and 0-speed landed ship rejections
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.add_fuel(1000.0);
    s.resource() = s.max_resource_capacity() + 50;
  });
  ctx.assert_dispatch_rejected(g, {"launch", "#1"});
  test::expect_contains(g.out.str(), "too overloaded to launch");

  const auto zero_speed_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                                 .owned_by(1, 0)
                                 .landed_on(0, 0, {5, 5})
                                 .with_max_speed(0)
                                 .build();
  ctx.assert_dispatch_rejected(
      g, {"launch", std::format("#{}", zero_speed_id.value)});
  test::expect_contains(g.out.str(), "not designed to be launched");
}

}  // namespace

int main() {
  test_launch_happy_paths();
  test_launch_canister_ships();
  test_launch_insufficient_ap();
  test_launch_domain_errors();
  test_launch_from_carrier_all_scopes();
  test_launch_planet_fuel_precheck_preserves_ap_and_coords();

  std::println(std::cout, "✓ launch_test passed!");
  return 0;
}
