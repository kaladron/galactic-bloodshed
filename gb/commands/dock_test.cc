// SPDX-License-Identifier: Apache-2.0

/// \file dock_test.cc
/// \brief Unit tests for dock and assault commands

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Ship 1: Player 1 Fighter
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(1, 0)
      .named("Docker")
      .in_star_orbit(1, 100.0, 200.0)
      .with_crew(0, 10)
      .with_fuel(100.0)
      .build();

  // Ship 2: Player 1 Carrier (close to ship 1)
  TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
      .owned_by(1, 0)
      .named("Carrier")
      .in_star_orbit(1, 100.0, 200.0)
      .with_fuel(100.0)
      .build();

  // Ship 3: Player 2 Cargo Ship (target for assault)
  TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
      .owned_by(2, 0)
      .named("Target")
      .in_star_orbit(1, 100.0, 200.0)
      .with_fuel(100.0)
      .build();

  // Ship 4: Far away ship
  TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
      .owned_by(1, 0)
      .named("FarTarget")
      .in_star_orbit(1, 500.0, 500.0)
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
  g.set_snum(1);

  // 1. Successful dock (0 AP)
  ctx.assert_dispatch_success(g, {"dock", "#1", "#2"}, 0);
  test::expect_contains(g.out.str(), "docked with");

  const auto* s1 = ctx.em.peek_ship(1);
  const auto* s2 = ctx.em.peek_ship(2);
  test::expect_true(s1 != nullptr);
  test::expect_true(s2 != nullptr);
  test::expect_eq(s1->docked(), 1);
  test::expect_true(s1->is_docked());
  test::expect_false(s1->is_landed());
  test::expect_eq(s1->whatdest(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(s1->destshipno(), 2);
  test::expect_eq(s2->docked(), 1);
  test::expect_true(s2->is_docked());
  test::expect_false(s2->is_landed());
  test::expect_eq(s2->whatdest(), ScopeLevel::LEVEL_SHIP);
  test::expect_eq(s2->destshipno(), 1);

  // 2. Successful assault (1 AP deducted via dynamic AP)
  // Undock first for assault test
  ctx.em.mutate_ship(1, [](Ship& s) { s.undock_from_ship(); });
  ctx.assert_dispatch_success(g, {"assault", "#1", "#3"}, 1);
  test::expect_true(g.out.str().contains("VICTORY") ||
                    g.out.str().contains("CAPTURED"));

  ctx.verify_universe_invariants();
}

void test_assault_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0 for Player 1
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_assault_guest_rejection() {
  TestContext ctx;
  setup_test_world(ctx);
  ctx.em.mutate_race(2, [](Race& r) { r.Guest = true; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 2, 0);  // Player 2 is guest
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

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
  g.set_snum(1);

  // 1. Min args violation (< 3 args)
  ctx.assert_dispatch_rejected(g, {"dock", "#1"});
  test::expect_contains(g.out.str(), "Syntax: dock <ship> <target_ship>");

  // 2. Docking with self
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#1"});
  test::expect_contains(g.out.str(), "You can't dock with yourself!");

  // 3. Out of range docking
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#4"});
  test::expect_contains(g.out.str(), "10.00 or closer");

  // 4. Docking with enemy-owned ship (unauthorized)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#3"});
  test::expect_contains(g.out.str(), "You are not authorized to do this.");

  // 5. Non-existent target ship
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#9999"});
  test::expect_contains(g.out.str(), "The ship wasn't found.");

  // 6. Invalid target ship number string
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "invalid"});
  test::expect_contains(g.out.str(), "Invalid ship number.");

  // 7. Ships in different scopes
  ctx.em.mutate_ship(2,
                     [](Ship& s) { s.whatorbits() = ScopeLevel::LEVEL_UNIV; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#2"});
  test::expect_contains(g.out.str(), "Those ships are not in the same scope.");
  ctx.em.mutate_ship(2,
                     [](Ship& s) { s.whatorbits() = ScopeLevel::LEVEL_STAR; });

  // 8. Insufficient fuel
  ctx.em.mutate_ship(1, [](Ship& s) { s.admin_override_fuel(0.0); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#2"});
  test::expect_contains(g.out.str(), "Not enough fuel.");
  ctx.em.mutate_ship(1, [](Ship& s) { s.admin_override_fuel(100.0); });

  // 9. Irradiated/inactive ship cannot dock
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.active() = false;
    s.admin_override_radiation(100);
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#2"});
  test::expect_contains(g.out.str(), "is irradiated 100% and inactive.");
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.active() = true;
    s.admin_override_radiation(0);
  });

  // 10. Hyper-drive deactivation on dock
  ctx.em.mutate_ship(1, [](Ship& s) { s.hyper_drive().on = 1; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"dock", "#1", "#2"}, 0);
  test::expect_contains(g.out.str(), "Hyper-drive deactivated.");
  test::expect_eq(ctx.em.peek_ship(1)->hyper_drive().on, 0);

  // 11. Already docked ship cannot dock again
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#2"});
  test::expect_contains(g.out.str(), "is already docked.");

  ctx.verify_universe_invariants();
}

void test_assault_validation_and_ap_invariants() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // Set star AP to 5 to verify failed assaults do NOT deduct AP
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 5; });

  // 1. Invalid population type ("aliens")
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3", "5", "aliens"});
  test::expect_contains(g.out.str(), "Assault with what?");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 5);

  // 2. Pods cannot assault
  shipnum_t pod_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                         .owned_by(1, 0)
                         .named("SporePod")
                         .in_star_orbit(1, 100.0, 200.0)
                         .with_crew(0, 5)
                         .with_fuel(100.0)
                         .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"assault", std::format("#{}", pod_id.value), "#3"});
  test::expect_contains(g.out.str(), "Sorry. Pods cannot be used to assault.");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 5);

  // 3. No civilians on ship when assaulting with "civ"
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3", "5", "civ"});
  test::expect_contains(g.out.str(),
                        "You have no crew on this ship to assault with.");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 5);

  // 4. No troops on ship when assaulting with "mil"
  ctx.em.mutate_ship(1, [](Ship& s) { s.troops() = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3", "5", "mil"});
  test::expect_contains(g.out.str(),
                        "You have no troops on this ship to assault with.");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 5);
  ctx.em.mutate_ship(1, [](Ship& s) { s.troops() = 10; });

  // 5. Cannot assault Von Neumann machines
  shipnum_t vn_id = TestShipBuilder(ctx.em, ShipType::OTYPE_VN)
                        .owned_by(2, 0)
                        .named("VNProbe")
                        .in_star_orbit(1, 100.0, 200.0)
                        .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"assault", "#1", std::format("#{}", vn_id.value)});
  test::expect_contains(g.out.str(), "You can't assault Von Neumann machines.");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 5);

  // 6. Cannot use a docked ship or hangar-berthed ship to assault
  ctx.em.mutate_ship(1, [](Ship& s) { s.dock_with_ship(2); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3"});
  test::expect_contains(g.out.str(), "Your ship is already docked.");
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.whatorbits() = ScopeLevel::LEVEL_SHIP;
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3"});
  test::expect_contains(g.out.str(), "Your ship is landed on another ship.");
  ctx.em.mutate_ship(1,
                     [](Ship& s) { s.whatorbits() = ScopeLevel::LEVEL_STAR; });
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 5);

  ctx.verify_universe_invariants();
}

void test_assault_combat_boobytrap_and_unmooring() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1a. Repulsed boarding assault where attacker survives: moderate defense
  ctx.em.mutate_race(1, [](Race& r) { r.fighters = 10; });
  ctx.em.mutate_race(2, [](Race& r) { r.fighters = 10; });
  ctx.em.mutate_ship(3, [](Ship& s) {
    s.max_crew() = 100;
    s.troops() = 3;
    s.popn() = 0;
  });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"assault", "#1", "#3", "1", "military"}, 1);
  test::expect_contains(g.out.str(), "The boarding was repulsed; try again.");
  test::expect_eq(ctx.em.peek_ship(3)->owner(), 2);

  // 1b. Overwhelming defense destroys attacking ship during boarding
  ctx.em.mutate_ship(3, [](Ship& s) {
    s.troops() = 80;
    s.popn() = 20;
  });
  shipnum_t doomed_id = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                            .owned_by(1, 0)
                            .named("DoomedFighter")
                            .in_star_orbit(1, 100.0, 200.0)
                            .with_crew(0, 2)
                            .with_fuel(100.0)
                            .build();
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"assault", std::format("#{}", doomed_id.value), "#3", "2", "mil"}, 1);
  test::expect_contains(g.out.str(),
                        "The assault was too much for your bucket of bolts.");

  // 2. Assaulting spaceborne-moored ship unmoors it first
  shipnum_t partner_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                             .owned_by(2, 0)
                             .named("MooredPartner")
                             .in_star_orbit(1, 100.0, 200.0)
                             .build();
  ctx.em.mutate_ship(3, [&](Ship& s) {
    s.troops() = 0;
    s.popn() = 0;
    s.dock_with_ship(partner_id);
  });
  ctx.em.mutate_ship(partner_id, [&](Ship& s) { s.dock_with_ship(3); });
  ctx.em.mutate_ship(1, [](Ship& s) { s.troops() = 20; });

  g.out.str("");
  ctx.assert_dispatch_success(g, {"assault", "#1", "#3", "5", "mil"}, 1);
  test::expect_contains(g.out.str(), "VICTORY! the ship is yours!");
  test::expect_eq(ctx.em.peek_ship(3)->owner(), 1);
  test::expect_eq(ctx.em.peek_ship(partner_id)->docked(), 0);

  // 3. Boobytrapped unmanned robot ship inflicts boobytrap damage on attacker
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.admin_override_damage(0);
    s.troops() = 20;
  });
  shipnum_t mine_id = TestShipBuilder(ctx.em, ShipType::STYPE_MINE)
                          .owned_by(2, 0)
                          .named("BoobyMine")
                          .in_star_orbit(1, 100.0, 200.0)
                          .with_max_crew(0)
                          .with_destruct(10)
                          .build();
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"assault", "#1", std::format("#{}", mine_id.value)}, 1);
  test::expect_contains(g.out.str(), "Their boobytrap gave you");
  test::expect_contains(g.out.str(), "VICTORY! the ship is yours!");

  // 4. Universe scope assault (deducts Universe AP)
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.whatorbits() = ScopeLevel::LEVEL_UNIV;
    s.troops() = 20;
  });
  shipnum_t univ_target = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                              .owned_by(2, 0)
                              .named("UnivCargo")
                              .in_star_orbit(1, 100.0, 200.0)
                              .build();
  ctx.em.mutate_ship(univ_target,
                     [](Ship& s) { s.whatorbits() = ScopeLevel::LEVEL_UNIV; });
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.em.mutate_universe([](universe_struct& u) { u.AP[1] = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"assault", "#1", std::format("#{}", univ_target.value)});
  test::expect_contains(g.out.str(), "You need 1 universe action point.");

  ctx.em.mutate_universe([](universe_struct& u) { u.AP[1] = 5; });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"assault", "#1", std::format("#{}", univ_target.value)}, 0);
  test::expect_eq(ctx.em.peek_universe()->AP[1], 4);

  // 5. Civilian boarding assault (victory with casualties and morale gain)
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.whatorbits() = ScopeLevel::LEVEL_STAR;
    s.popn() = 30;
    s.troops() = 0;
  });
  shipnum_t civ_target = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                             .owned_by(2, 0)
                             .named("CivTarget")
                             .in_star_orbit(1, 100.0, 200.0)
                             .with_crew(2, 0)
                             .build();
  ctx.em.mutate_race(1, [](Race& r) { r.fighters = 15; });
  ctx.em.mutate_race(2, [](Race& r) { r.fighters = 1; });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"assault", "#1", std::format("#{}", civ_target.value), "20", "civ"},
      1);
  test::expect_contains(g.out.str(), "VICTORY! the ship is yours!");
  test::expect_eq(ctx.em.peek_ship(civ_target)->owner(), 1);

  // 6. Illegal boarder count (0 boarders requested)
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.troops() = 20;
  });
  shipnum_t zero_target = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                              .owned_by(2, 0)
                              .named("ZeroTarget")
                              .in_star_orbit(1, 100.0, 200.0)
                              .with_crew(5, 5)
                              .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"assault", "#1", std::format("#{}", zero_target.value), "0", "mil"});
  test::expect_contains(g.out.str(), "Illegal number of boarders (0).");

  // 7. Defender ship destroyed during boarding combat
  ctx.em.mutate_ship(zero_target, [](Ship& s) {
    s.admin_override_damage(98);
    s.popn() = 5;
    s.troops() = 0;
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"assault", "#1", std::format("#{}", zero_target.value), "20", "mil"},
      1);
  test::expect_contains(g.out.str(), "Their ship DESTROYED!!!");

  // 8. Target ship already landed/in hangar cannot be assaulted
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.whatorbits() = ScopeLevel::LEVEL_PLAN;
    s.pnumorbits() = 1;
    s.troops() = 20;
  });
  shipnum_t landed_target = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                                .owned_by(2, 0)
                                .named("LandedTarget")
                                .landed_on(1, 1, {1, 1})
                                .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"assault", "#1", std::format("#{}", landed_target.value)});
  test::expect_contains(g.out.str(), "is already docked.");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_dock_happy_paths();
  test_assault_insufficient_ap();
  test_assault_guest_rejection();
  test_dock_domain_errors();
  test_assault_validation_and_ap_invariants();
  test_assault_combat_boobytrap_and_unmooring();

  std::println(std::cout, "✓ dock_test passed!");
  return 0;
}
