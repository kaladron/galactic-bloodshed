// SPDX-License-Identifier: Apache-2.0

/// \file dock_test.cc
/// \brief Unit tests for peaceful dock command and dock_single_ship mechanics.

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

  // Ship 3: Player 2 Cargo Ship
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

void test_dock_happy_paths_and_json_presentation() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
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

  // 2. JSON presentation mode for dock
  ctx.em.mutate_ship(1, [](Ship& s) { s.undock_from_ship(); });
  ctx.em.mutate_ship(2, [](Ship& s) { s.undock_from_ship(); });
  g.set_ui_mode(UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"dock", "#1", "#2"}, 0);
  test::expect_contains(g.out.str(), "\"type\":\"peaceful_dock\"");

  ctx.verify_universe_invariants();
}

void test_dock_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Min args violation (< 3 args) and direct handler invocation
  ctx.assert_dispatch_rejected(g, {"dock", "#1"});
  test::expect_contains(g.out.str(), "Syntax: dock <ship> <target_ship>");

  g.out.str("");
  test::expect_false(GB::commands::dock({"dock"}, g));
  test::expect_contains(g.out.str(), "Dock with what?");

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
  ctx.em.mutate_ship(2, [](Ship& s) { s.enter_deep_space(); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#2"});
  test::expect_contains(g.out.str(), "Those ships are not in the same scope.");
  ctx.em.mutate_ship(2, [](Ship& s) { s.enter_star_orbit(1); });

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

  // 10. Subordinate governor (gov 2) cannot dock a ship assigned to governor 1
  g.set_governor(2);
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#2"});
  g.set_governor(1);

  // 11. Hyper-drive deactivation on dock
  ctx.em.mutate_ship(1, [](Ship& s) { s.hyper_drive().on = 1; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"dock", "#1", "#2"}, 0);
  test::expect_contains(g.out.str(), "Hyper-drive deactivated.");
  test::expect_eq(ctx.em.peek_ship(1)->hyper_drive().on, 0);

  // 12. Already docked ship cannot dock again
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"dock", "#1", "#2"});
  test::expect_contains(g.out.str(), "is already docked.");

  // 13. Another undocked ship attempting to dock with already-docked target #2
  shipnum_t pod_docker = TestShipBuilder(ctx.em, ShipType::STYPE_POD)
                             .owned_by(1, 1)
                             .named("PodDocker")
                             .in_star_orbit(1, 100.0, 200.0)
                             .with_fuel(100.0)
                             .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"dock", std::format("#{}", pod_docker.value), "#2"});
  test::expect_contains(g.out.str(), "is already docked.");

  // 14. God mode allows docking foreign ships together
  ctx.em.mutate_ship(1, [](Ship& s) { s.undock_from_ship(); });
  ctx.em.mutate_ship(2, [](Ship& s) { s.undock_from_ship(); });
  ctx.em.mutate_race(1, [](Race& r) { r.God = true; });
  g.set_god(true);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"dock", "#3", "#2"}, 0);
  test::expect_contains(g.out.str(), "docked with");

  // 15. Multi-ship filter ('f'): first fighter (#1) is too far away (continues
  // loop), second fighter docks with #2, and third fighter aborts on
  // TargetAlreadyDocked.
  ctx.em.mutate_ship(3, [](Ship& s) { s.undock_from_ship(); });
  ctx.em.mutate_ship(2, [](Ship& s) { s.undock_from_ship(); });
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.undock_from_ship();
    s.set_coordinates({500.0, 500.0});
  });
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(1, 1)
      .named("NearFighter")
      .in_star_orbit(1, 100.0, 200.0)
      .with_fuel(100.0)
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_FIGHTER)
      .owned_by(1, 1)
      .named("ThirdFighter")
      .in_star_orbit(1, 100.0, 200.0)
      .with_fuel(100.0)
      .build();
  g.set_god(false);
  ctx.em.mutate_race(1, [](Race& r) { r.God = false; });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"dock", "f", "#2"}, 0);
  test::expect_contains(g.out.str(), "10.00 or closer");
  test::expect_contains(g.out.str(), "docked with");
  test::expect_contains(g.out.str(), "is already docked.");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_dock_happy_paths_and_json_presentation();
  test_dock_domain_errors();

  std::println(std::cout, "✓ dock_test passed!");
  return 0;
}
