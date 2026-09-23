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
  ctx.with_standard_universe();

  // Create a ship that can land (shuttle)
  TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
      .owned_by(1, 0)
      .named("TestShuttle")
      .in_planet_orbit(1, 1)
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
  g.set_snum(1);
  g.set_pnum(1);
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
  g.set_snum(1);
  g.set_pnum(1);

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

  // Reset shuttle to spaceborne state with land_coords at 5,5
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
    s.set_land_coords({5, 5});
  });

  // Create a carrier landed at (5, 5)
  TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
      .owned_by(1, 0)
      .named("TestCarrier")
      .landed_on(1, 1, Coordinates(5, 5))
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

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
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

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
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Min args check (< 3 args)
  ctx.assert_dispatch_rejected(g, {"land", "#1"});
  test::expect_contains(g.out.str(), "Syntax: land <ship> <#mothership | x,y>");

  // 2. Invalid coordinates format
  ctx.assert_dispatch_rejected(g, {"land", "#1", "bad_coords"});
  test::expect_contains(g.out.str(), "Invalid coordinates format");

  ctx.verify_universe_invariants();
}

void test_land_spaceborne_on_carrier_and_edge_cases() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Carrier in planet orbit at same coordinates as shuttle #1
  const auto carrier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                              .owned_by(1, 0)
                              .named("OrbitCarrier")
                              .in_planet_orbit(1, 1)
                              .with_max_hanger(100)
                              .build();

  // 1. Non-existent target ship
  ctx.assert_dispatch_rejected(g, {"land", "#1", "#999"});
  test::expect_contains(g.out.str(), "wasn't found");

  // 2. Invalid ship string
  ctx.assert_dispatch_rejected(g, {"land", "#1", "#abc"});
  test::expect_contains(g.out.str(), "wasn't found");

  // 3. Factory target rejected
  const auto factory_id = TestShipBuilder(ctx.em, ShipType::OTYPE_FACTORY)
                              .owned_by(1, 0)
                              .in_planet_orbit(1, 1)
                              .build();
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", factory_id.value)});
  test::expect_contains(g.out.str(), "Can't land on factories.");

  // 4. Foreign carrier rejected
  const auto enemy_carrier = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                                 .owned_by(2, 0)
                                 .in_planet_orbit(1, 1)
                                 .build();
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", enemy_carrier.value)});
  test::expect_contains(g.out.str(), "Illegal format.");

  // 5. Different scope rejected
  ctx.em.mutate_ship(
      carrier_id, [](Ship& c) { c.launch_to_orbit(ScopeLevel::LEVEL_STAR); });
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "not in the same scope");

  // 6. Universe scope rejected WITHOUT losing fuel
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.em.mutate_ship(
      1, [](Ship& s) { s.launch_to_orbit(ScopeLevel::LEVEL_UNIV); });
  ctx.em.mutate_ship(
      carrier_id, [](Ship& c) { c.launch_to_orbit(ScopeLevel::LEVEL_UNIV); });
  const double fuel_before = ctx.em.peek_ship(1)->fuel();
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "not in planet or star scope");
  test::expect_eq(ctx.em.peek_ship(1)->fuel(), fuel_before);

  // Restore both to planet orbit, test distance > DIST_TO_DOCK
  g.set_level(ScopeLevel::LEVEL_PLAN);
  ctx.em.mutate_ship(1, [&](Ship& s) {
    s.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
    s.set_coordinates({0.0, 0.0});
  });
  ctx.em.mutate_ship(carrier_id, [](Ship& c) {
    c.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
    c.set_coordinates({50.0, 0.0});
  });
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "or closer to");

  // Move carrier close, test insufficient fuel
  ctx.em.mutate_ship(carrier_id,
                     [](Ship& c) { c.set_coordinates({1.0, 0.0}); });
  ctx.em.mutate_ship(1, [](Ship& s) { s.consume_fuel(s.fuel()); });
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "Not enough fuel.");

  // Restore fuel, test insufficient hangar space
  ctx.em.mutate_ship(1, [](Ship& s) { s.add_fuel(20.0); });
  ctx.em.mutate_ship(carrier_id, [](Ship& c) { c.max_hanger() = 0; });
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "hanger space");

  // Restore hangar space and succeed
  ctx.em.mutate_ship(carrier_id, [](Ship& c) { c.max_hanger() = 100; });
  ctx.assert_dispatch_success(
      g, {"land", "#1", std::format("#{}", carrier_id.value)}, 0);
  test::expect_contains(g.out.str(), "landed on");
  test::expect_eq(ctx.em.peek_ship(1)->whatorbits(), ScopeLevel::LEVEL_SHIP);
}

void test_land_mothership_loading_edge_cases() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Land shuttle #1 at (5, 5)
  ctx.assert_dispatch_success(g, {"land", "#1", "5,5"}, 1);

  const auto carrier_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
                              .owned_by(1, 0)
                              .named("GroundCarrier")
                              .in_planet_orbit(1, 1)
                              .with_max_hanger(100)
                              .build();

  // 1. Mothership not landed
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "is not landed on a planet");

  // 2. Different star system
  ctx.em.mutate_ship(carrier_id, [](Ship& c) {
    c.land_on_planet();
    c.storbits() = 2;
    c.pnumorbits() = 1;
    c.set_land_coords({5, 5});
  });
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "not in the same star system");

  // 3. Different planet
  ctx.em.mutate_ship(carrier_id, [](Ship& c) {
    c.storbits() = 1;
    c.pnumorbits() = 2;
  });
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "not landed on the same planet");

  // 4. Different sector
  ctx.em.mutate_ship(carrier_id, [](Ship& c) {
    c.pnumorbits() = 1;
    c.set_land_coords({4, 4});
  });
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "not in the same sector");

  // 5. Powered-on ship rejected
  ctx.em.mutate_ship(carrier_id, [](Ship& c) { c.set_land_coords({5, 5}); });
  ctx.em.mutate_ship(1, [](Ship& s) { s.on() = true; });
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "must be turned off before loading");

  // 6. Insufficient hangar space
  ctx.em.mutate_ship(1, [](Ship& s) { s.on() = false; });
  ctx.em.mutate_ship(carrier_id, [](Ship& c) { c.max_hanger() = 0; });
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "hanger space");

  // 7. Overloaded, Quarry, and Ship-to-Ship Docked rejections in land()
  ctx.em.mutate_ship(
      1, [](Ship& s) { s.resource() = s.max_resource_capacity() + 100; });
  ctx.assert_dispatch_rejected(
      g, {"land", "#1", std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "too overloaded to land");

  const auto quarry_id = TestShipBuilder(ctx.em, ShipType::OTYPE_QUARRY)
                             .owned_by(1, 0)
                             .landed_on(1, 1, {5, 5})
                             .build();
  ctx.assert_dispatch_rejected(g, {"land", std::format("#{}", quarry_id.value),
                                   std::format("#{}", carrier_id.value)});
  test::expect_contains(g.out.str(), "can't load quarries");
}

void test_land_planet_preconditions_and_crashes() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Out-of-bounds coordinates do NOT deduct Star AP
  const ap_t ap_before = ctx.em.peek_star(1)->AP(1);
  ctx.assert_dispatch_rejected(g, {"land", "#1", "99,99"});
  test::expect_contains(g.out.str(), "Illegal coordinates.");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), ap_before);

  // 2. Distance > DIST_TO_LAND does NOT deduct Star AP
  ctx.em.mutate_ship(1, [](Ship& s) { s.set_coordinates({500.0, 500.0}); });
  ctx.assert_dispatch_rejected(g, {"land", "#1", "5,5"});
  test::expect_contains(g.out.str(), "or closer to the planet");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), ap_before);

  // 3. Wasteland and allied/alien sector messages + defense fire
  const auto* star = ctx.em.peek_star(1);
  const auto* planet = ctx.em.peek_planet(1, 1);
  const auto planet_coords = planet->absolute_coordinates(*star);
  ctx.em.mutate_ship(1, [&](Ship& s) { s.set_coordinates(planet_coords); });
  ctx.em.mutate_sectormap(1, 1, [](SectorMap& smap) {
    smap.get({2, 2}).set_condition(SectorType::SEC_WASTED);
    smap.get({3, 3}).set_owner(2);
  });
  ctx.assert_dispatch_success(g, {"land", "#1", "2,2"}, 1);
  test::expect_contains(g.out.str(), "Warning: That sector is a wasteland!");

  // Reset to orbit and land on alien sector (3, 3) (not allied)
  ctx.em.mutate_ship(1, [&](Ship& s) {
    s.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
    s.set_coordinates(planet_coords);
    s.add_fuel(50.0);
  });
  ctx.em.mutate_planet(1, 1, [](Planet& p) { p.info(2).numsectsowned = 1; });
  ctx.assert_dispatch_success(g, {"land", "#1", "3,3"}, 1);
  test::expect_contains(g.out.str(), "You have landed on an alien sector");

  // Reset to orbit and land on allied sector (3, 3) (mutual alliance)
  ctx.em.mutate_ship(1, [&](Ship& s) {
    s.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
    s.set_coordinates(planet_coords);
    s.add_fuel(50.0);
  });
  ctx.em.mutate_race(1, [](Race& r) { r.declare_alliance_with(2); });
  ctx.em.mutate_race(2, [](Race& r) { r.declare_alliance_with(1); });
  g.race = ctx.em.peek_race(1);
  ctx.assert_dispatch_success(g, {"land", "#1", "3,3"}, 1);
  test::expect_contains(g.out.str(), "You have landed on allied sector");

  // Trigger planetary defense fire when at war
  ctx.em.mutate_ship(1, [&](Ship& s) {
    s.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
    s.set_coordinates(planet_coords);
    s.add_fuel(50.0);
  });
  ctx.em.mutate_race(2, [](Race& r) { r.declare_war_on(1); });
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(2).popn = 500;
    p.info(2).guns = 5;
    p.info(2).destruct = 5;
  });
  ctx.assert_dispatch_success(g, {"land", "#1", "3,3"}, 1);
  test::expect_eq(ctx.em.peek_planet(1, 1)->info(2).destruct, 0);

  // 4. Crash from insufficient fuel (persists ship destruction & deducts 1 AP)
  const auto crash_fuel_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                                 .owned_by(1, 0)
                                 .in_planet_orbit(1, 1)
                                 .with_fuel(0.0)
                                 .build();
  ctx.assert_dispatch_success(
      g, {"land", std::format("#{}", crash_fuel_id.value), "5,5"}, 1);
  test::expect_contains(g.out.str(), "while the landing required");
  test::expect_false(ctx.em.peek_ship(crash_fuel_id)->alive());

  // 5. Crash from 100% hull damage (persists ship destruction & deducts 1 AP)
  const auto crash_dmg_id = TestShipBuilder(ctx.em, ShipType::STYPE_SHUTTLE)
                                .owned_by(1, 0)
                                .in_planet_orbit(1, 1)
                                .with_fuel(20.0)
                                .with_damage(100)
                                .build();
  ctx.assert_dispatch_success(
      g, {"land", std::format("#{}", crash_dmg_id.value), "5,5"}, 1);
  test::expect_contains(g.out.str(), "Ship damage 100%");
  test::expect_false(ctx.em.peek_ship(crash_dmg_id)->alive());
}

}  // namespace

int main() {
  test_land_on_planet();
  test_cannot_land_docked_ship();
  test_land_on_friendly_carrier();
  test_land_insufficient_ap();
  test_land_domain_errors();
  test_land_spaceborne_on_carrier_and_edge_cases();
  test_land_mothership_loading_edge_cases();
  test_land_planet_preconditions_and_crashes();

  std::println(std::cout, "✓ land_test passed!");
  return 0;
}
