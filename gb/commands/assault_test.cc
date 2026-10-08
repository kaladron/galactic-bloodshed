// SPDX-License-Identifier: Apache-2.0

/// \file assault_test.cc
/// \brief Unit tests for hostile boarding assault command,
/// execute_defensive_fire, and assault_single_ship mechanics.

import commands;
import dallib;
import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Ship 1: Player 1 Fighter
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(1, 1)
      .named("Docker")
      .in_star_orbit(1, 100.0, 200.0)
      .with_crew(0, 10)
      .with_fuel(100.0)
      .build();

  // Ship 2: Player 1 Carrier (close to ship 1)
  TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
      .owned_by(1, 1)
      .named("Carrier")
      .in_star_orbit(1, 100.0, 200.0)
      .with_fuel(100.0)
      .build();

  // Ship 3: Player 2 Cargo Ship (target for assault)
  TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
      .owned_by(2, 1)
      .named("Target")
      .in_star_orbit(1, 100.0, 200.0)
      .with_destruct(0)
      .with_fuel(100.0)
      .build();

  // Ship 4: Far away ship
  TestShipBuilder(ctx.em, ShipType::STYPE_CARRIER)
      .owned_by(1, 1)
      .named("FarTarget")
      .in_star_orbit(1, 500.0, 500.0)
      .with_fuel(100.0)
      .build();
}

void test_assault_happy_path_and_json_presentation() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Successful assault (1 AP deducted via dynamic AP)
  ctx.assert_dispatch_success(g, {"assault", "#1", "#3"}, 1);
  test::expect_contains(g.out.str(), "Damage taken:  You:");
  test::expect_true(g.out.str().contains("VICTORY") ||
                    g.out.str().contains("CAPTURED"));
  test::expect_eq(ctx.em.peek_ship(3)->owner(), 1);

  // 2. JSON presentation mode for assault
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.troops() = 10;
  });
  ctx.em.mutate_ship(3, [](Ship& s) {
    s.undock_from_ship();
    s.owner() = 2;
    s.troops() = 0;
    s.popn() = 0;
  });
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"assault", "#1", "#3"}, 1);
  test::expect_contains(g.out.str(), "\"type\":\"assault\"");

  ctx.verify_universe_invariants();
}

void test_assault_insufficient_ap_and_guest_rejection() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0 for Player 1
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3"});
  test::expect_contains(g.out.str(), "action points");

  // Guest race rejection
  ctx.em.mutate_race(2, [](Race& r) { r.Guest = true; });
  GameObj g2(ctx.em, registry);
  ctx.setup_game_obj(g2, 2, 1);
  g2.set_level(ScopeLevel::LEVEL_STAR);
  g2.set_snum(1);

  ctx.assert_dispatch_rejected(g2, {"assault", "#3", "#1"});
  test::expect_contains(g2.out.str(), "Guest races cannot use this command.");

  ctx.verify_universe_invariants();
}

void test_assault_validation_and_ap_invariants() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // Set star AP to 5 to verify failed assaults do NOT deduct AP
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 5; });

  // 0. Direct handler missing args check
  g.out.str("");
  test::expect_false(GB::commands::assault({"assault"}, g));
  test::expect_contains(g.out.str(), "Assault what?");

  // 1. Invalid population type ("aliens")
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3", "5", "aliens"});
  test::expect_contains(g.out.str(), "Assault with what?");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 5);

  // 2. Pods cannot assault
  shipnum_t pod_id = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                         .owned_by(1, 1)
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
                        .owned_by(2, 1)
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
  ctx.em.mutate_ship(1, [](Ship& s) { s.dock_into_carrier(2); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3"});
  test::expect_contains(g.out.str(), "Your ship is landed on another ship.");
  ctx.em.mutate_ship(1, [](Ship& s) { s.enter_star_orbit(1); });
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 5);

  // 7. Irradiated ship, self-assault, non-existent target, invalid ship number,
  // different scope, too far away, insufficient fuel
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.active() = false;
    s.admin_override_radiation(85);
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3"});
  test::expect_contains(g.out.str(), "is irradiated 85% and inactive.");
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.active() = true;
    s.admin_override_radiation(0);
  });

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#1"});
  test::expect_contains(g.out.str(), "You can't dock with yourself!");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#9999"});
  test::expect_contains(g.out.str(), "The ship wasn't found.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "badnum"});
  test::expect_contains(g.out.str(), "Invalid ship number.");

  ctx.em.mutate_ship(3, [](Ship& s) { s.enter_deep_space(); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3"});
  test::expect_contains(g.out.str(), "Those ships are not in the same scope.");
  ctx.em.mutate_ship(3, [](Ship& s) { s.enter_star_orbit(1); });

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#4"});
  test::expect_contains(g.out.str(), "10.00 or closer");

  ctx.em.mutate_ship(1, [](Ship& s) { s.admin_override_fuel(0.0); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"assault", "#1", "#3"});
  test::expect_contains(g.out.str(), "Not enough fuel.");
  ctx.em.mutate_ship(1, [](Ship& s) { s.admin_override_fuel(100.0); });

  ctx.verify_universe_invariants();
}

void test_assault_combat_boobytrap_and_unmooring() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
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
                            .owned_by(1, 1)
                            .named("DoomedFighter")
                            .in_star_orbit(1, 100.0, 200.0)
                            .with_damage(99)
                            .with_crew(0, 2)
                            .with_fuel(100.0)
                            .build();
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"assault", std::format("#{}", doomed_id.value), "#3", "2", "mil"}, 1);
  test::expect_contains(g.out.str(),
                        "The assault was too much for your bucket of bolts.");

  // 2. Assaulting spaceborne-moored ship unmoors it first (and hyperdrive off)
  shipnum_t partner_id = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                             .owned_by(2, 1)
                             .named("MooredPartner")
                             .in_star_orbit(1, 100.0, 200.0)
                             .build();
  ctx.em.mutate_ship(3, [&](Ship& s) {
    s.troops() = 0;
    s.popn() = 0;
    s.dock_with_ship(partner_id);
  });
  ctx.em.mutate_ship(partner_id, [&](Ship& s) { s.dock_with_ship(3); });
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.troops() = 20;
    s.hyper_drive().on = 1;
  });

  g.out.str("");
  ctx.assert_dispatch_success(g, {"assault", "#1", "#3", "5", "mil"}, 1);
  test::expect_contains(g.out.str(), "Hyper-drive deactivated.");
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
                          .owned_by(2, 1)
                          .named("BoobyMine")
                          .in_star_orbit(1, 100.0, 200.0)
                          .with_max_crew(0)
                          .with_destruct(5)
                          .build();
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"assault", "#1", std::format("#{}", mine_id.value)}, 1);
  test::expect_contains(g.out.str(), "Their boobytrap gave you");
  test::expect_contains(g.out.str(), "VICTORY! the ship is yours!");

  // 4. Universe scope assault (deducts Universe AP)
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.enter_deep_space();
    s.troops() = 20;
  });
  shipnum_t univ_target = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                              .owned_by(2, 1)
                              .named("UnivCargo")
                              .in_star_orbit(1, 100.0, 200.0)
                              .build();
  ctx.em.mutate_ship(univ_target, [](Ship& s) { s.enter_deep_space(); });
  g.set_level(ScopeLevel::LEVEL_UNIV);
  ctx.em.mutate_universe([](universe_struct& u) { u.set_AP(1, 0); });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"assault", "#1", std::format("#{}", univ_target.value)});
  test::expect_contains(g.out.str(), "You need 1 universe action point.");

  ctx.em.mutate_universe([](universe_struct& u) { u.set_AP(1, 5); });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"assault", "#1", std::format("#{}", univ_target.value)}, 0);
  test::expect_eq(ctx.em.peek_universe()->get_AP(1), 4);

  // 5. Civilian boarding assault (victory with civilian boarders)
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.enter_star_orbit(1);
    s.popn() = 30;
    s.troops() = 0;
  });
  shipnum_t civ_target = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                             .owned_by(2, 1)
                             .named("CivTarget")
                             .in_star_orbit(1, 100.0, 200.0)
                             .with_destruct(0)
                             .with_crew(0, 0)
                             .build();
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"assault", "#1", std::format("#{}", civ_target.value), "20", "civ"},
      1);
  test::expect_contains(g.out.str(), "VICTORY! the ship is yours!");
  test::expect_eq(ctx.em.peek_ship(civ_target)->owner(), 1);
  test::expect_eq(ctx.em.peek_ship(civ_target)->popn(), 20);

  // 6. Illegal boarder count (0 boarders requested)
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.troops() = 20;
  });
  shipnum_t zero_target = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                              .owned_by(2, 1)
                              .named("ZeroTarget")
                              .in_star_orbit(1, 100.0, 200.0)
                              .with_destruct(0)
                              .with_crew(5, 5)
                              .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"assault", "#1", std::format("#{}", zero_target.value), "0", "mil"});
  test::expect_contains(g.out.str(), "Illegal number of boarders (0).");

  // 7. Defender ship destroyed during boarding combat: boarders die on target
  // and do NOT return to attacking ship #1.
  ctx.em.mutate_ship(zero_target, [](Ship& s) {
    s.admin_override_damage(99);
    s.popn() = 1;
    s.troops() = 0;
  });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"assault", "#1", std::format("#{}", zero_target.value), "20", "mil"},
      1);
  test::expect_contains(g.out.str(),
                        "Their ship DESTROYED!!!  Boarders are dead.");
  test::expect_eq(ctx.em.peek_ship(1)->troops(), 0);

  // 8. Target ship already landed/in hangar cannot be assaulted
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.enter_planet_orbit(1, 1);
    s.troops() = 20;
  });
  shipnum_t landed_target = TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
                                .owned_by(2, 1)
                                .named("LandedTarget")
                                .landed_on(1, 1, {1, 1})
                                .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"assault", "#1", std::format("#{}", landed_target.value)});
  test::expect_contains(g.out.str(), "is already docked.");

  ctx.verify_universe_invariants();
}

void test_assault_defensive_fire_and_escort_retaliation() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 10; });

  // 1. Defensive gun fire from target destroys the attacking ship (!s.alive()).
  // Transaction must commit so AP is deducted and attacker stays dead, and
  // defending player (Player 2) receives the defensive fire telegram.
  shipnum_t gun_defender = TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
                               .owned_by(2, 1)
                               .named("GunDefender")
                               .in_star_orbit(1, 100.0, 200.0)
                               .with_guns(guntype_t::HEAVY, 10)
                               .with_crew(20, 20)
                               .with_fuel(500.0)
                               .build();
  ctx.em.mutate_ship(gun_defender, [](Ship& s) { s.tech() = 1000.0; });

  shipnum_t fragile_attacker = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                                   .owned_by(1, 1)
                                   .named("FragileAttacker")
                                   .in_star_orbit(1, 100.0, 200.0)
                                   .with_armor(0)
                                   .with_damage(50)
                                   .with_crew(0, 10)
                                   .with_fuel(100.0)
                                   .build();

  g.out.str("");
  ctx.assert_dispatch_success(g,
                              {"assault",
                               std::format("#{}", fragile_attacker.value),
                               std::format("#{}", gun_defender.value)},
                              1);
  test::expect_throws<EntityNotFoundError>(
      [&]() { ctx.em.peek_ship(fragile_attacker); });
  test::expect_true(ctx.em.peek_ship(gun_defender)->alive());
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 9);

  const auto p2_telegrams = ctx.em.get_telegrams(2, 1);
  test::expect_false(p2_telegrams.empty());

  // 2a. Target fires defensive guns, and attacker's self-defense retaliation
  // destroys the target ship (!s2.alive()). Transaction must commit.
  shipnum_t glass_defender = TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
                                 .owned_by(2, 1)
                                 .named("GlassDefender")
                                 .in_star_orbit(1, 100.0, 200.0)
                                 .with_guns(guntype_t::LIGHT, 1)
                                 .with_armor(0)
                                 .with_size(1)
                                 .with_damage(99)
                                 .with_crew(5, 5)
                                 .with_fuel(500.0)
                                 .build();
  ctx.em.mutate_ship(glass_defender, [](Ship& s) { s.tech() = 100000.0; });

  shipnum_t heavy_attacker = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                                 .owned_by(1, 1)
                                 .named("HeavyAttacker")
                                 .in_star_orbit(1, 100.0, 200.0)
                                 .with_guns(guntype_t::HEAVY, 100)
                                 .with_destruct(500)
                                 .with_armor(0)
                                 .with_size(10000)
                                 .with_max_hanger(0)
                                 .with_crew(200, 20)
                                 .with_fuel(500.0)
                                 .build();
  ctx.em.mutate_ship(heavy_attacker, [](Ship& s) {
    s.tech() = 1000.0;
    s.protect().retaliate = true;
  });

  g.out.str("");
  ctx.assert_dispatch_success(g,
                              {"assault",
                               std::format("#{}", heavy_attacker.value),
                               std::format("#{}", glass_defender.value)},
                              1);
  test::expect_true(ctx.em.peek_ship(heavy_attacker)->alive());
  test::expect_throws<EntityNotFoundError>(
      [&]() { ctx.em.peek_ship(glass_defender); });
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 8);

  // 2b. Target fires defensive guns at an attacker with self-retaliate off,
  // and an escort ship protecting the attacker retaliates.
  ctx.em.mutate_ship(heavy_attacker, [](Ship& s) {
    s.protect().retaliate = false;
    s.troops() = 20;
  });

  shipnum_t escort_ship = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                              .owned_by(1, 1)
                              .named("EscortShip")
                              .in_star_orbit(1, 100.0, 200.0)
                              .with_guns(guntype_t::HEAVY, 100)
                              .with_destruct(500)
                              .with_crew(200, 20)
                              .with_fuel(500.0)
                              .build();
  ctx.em.mutate_ship(escort_ship, [&](Ship& s) {
    s.tech() = 100000.0;
    s.protect().on = true;
    s.protect().ship = heavy_attacker;
  });

  shipnum_t escort_target = TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
                                .owned_by(2, 1)
                                .named("EscortTarget")
                                .in_star_orbit(1, 100.0, 200.0)
                                .with_guns(guntype_t::LIGHT, 1)
                                .with_destruct(10)
                                .with_armor(0)
                                .with_size(10000)
                                .with_max_hanger(0)
                                .with_damage(99)
                                .with_crew(5, 5)
                                .with_fuel(500.0)
                                .build();
  ctx.em.mutate_ship(escort_target, [](Ship& s) { s.tech() = 100000.0; });

  g.out.str("");
  ctx.assert_dispatch_success(g,
                              {"assault",
                               std::format("#{}", heavy_attacker.value),
                               std::format("#{}", escort_target.value)},
                              1);
  test::expect_throws<EntityNotFoundError>(
      [&]() { ctx.em.peek_ship(escort_target); });
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 7);

  // 3. Defensive laser fire with crystal mounted, plus laser self-retaliation
  // and planet-orbit laser escort retaliation
  ctx.em.mutate_ship(heavy_attacker, [](Ship& s) {
    s.undock_from_ship();
    s.enter_planet_orbit(1, 1);
    s.troops() = 20;
    s.laser() = 1;
    s.mounted() = 1;
    s.fire_laser() = 5;
    s.protect().retaliate = true;
  });
  ctx.em.mutate_ship(escort_ship, [&](Ship& s) {
    s.enter_planet_orbit(1, 1);
    s.laser() = 1;
    s.mounted() = 1;
    s.fire_laser() = 5;
    s.protect().on = true;
    s.protect().ship = heavy_attacker;
  });

  // Also add an inactive escort and an escort protecting a different ship in
  // planet orbit to exercise is_eligible_escort rejection branches
  shipnum_t inactive_escort =
      TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
          .owned_by(1, 1)
          .named("InactiveEscort")
          .in_planet_orbit(1, 1, UniverseCoordinates{100.0, 200.0})
          .with_crew(10, 10)
          .build();
  ctx.em.mutate_ship(inactive_escort, [&](Ship& s) {
    s.active() = false;
    s.protect().on = true;
    s.protect().ship = heavy_attacker;
  });

  shipnum_t other_escort =
      TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
          .owned_by(1, 1)
          .named("OtherEscort")
          .in_planet_orbit(1, 1, UniverseCoordinates{100.0, 200.0})
          .with_crew(10, 10)
          .build();
  ctx.em.mutate_ship(other_escort, [&](Ship& s) {
    s.protect().on = true;
    s.protect().ship = shipnum_t{1};
  });

  shipnum_t laser_defender =
      TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
          .owned_by(2, 1)
          .named("LaserDefender")
          .in_planet_orbit(1, 1, UniverseCoordinates{100.0, 200.0})
          .with_armor(0)
          .with_size(10000)
          .with_crew(20, 20)
          .with_fuel(500.0)
          .build();
  ctx.em.mutate_ship(laser_defender, [](Ship& s) {
    s.laser() = 1;
    s.mounted() = 1;
    s.fire_laser() = 5;
    s.tech() = 100000.0;
  });

  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_pnum(1);
  g.out.str("");
  ctx.assert_dispatch_success(g,
                              {"assault",
                               std::format("#{}", heavy_attacker.value),
                               std::format("#{}", laser_defender.value)},
                              1);
  test::expect_contains(g.out.str(), "Distance to");

  // 4. Defensive laser overload (high fire_laser strength triggers crystal
  // overload)
  ctx.em.mutate_ship(heavy_attacker, [](Ship& s) {
    s.undock_from_ship();
    s.troops() = 20;
    s.admin_override_damage(0);
  });
  shipnum_t overload_defender =
      TestShipBuilder(ctx.em, ShipType::STYPE_CRUISER)
          .owned_by(2, 1)
          .named("OverloadDefender")
          .in_planet_orbit(1, 1, UniverseCoordinates{100.0, 200.0})
          .with_crew(0, 0)
          .with_fuel(5000.0)
          .build();
  ctx.em.mutate_ship(overload_defender, [](Ship& s) {
    s.laser() = 1;
    s.mounted() = 1;
    s.fire_laser() = 500;
    s.tech() = 1.0;
  });

  g.out.str("");
  ctx.assert_dispatch_success(g,
                              {"assault",
                               std::format("#{}", heavy_attacker.value),
                               std::format("#{}", overload_defender.value)},
                              1);
  test::expect_contains(g.out.str(), "Distance to");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_assault_happy_path_and_json_presentation();
  test_assault_insufficient_ap_and_guest_rejection();
  test_assault_validation_and_ap_invariants();
  test_assault_combat_boobytrap_and_unmooring();
  test_assault_defensive_fire_and_escort_retaliation();

  std::println(std::cout, "✓ assault_test passed!");
  return 0;
}
