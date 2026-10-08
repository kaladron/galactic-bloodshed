// SPDX-License-Identifier: Apache-2.0

/// \file cew_test.cc
/// \brief Unit tests for cew command and Confined Energy Weapon mechanics.

import commands;
import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

void setup_cew_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Create attacker ship #1 - armed with CEW (power 10, range 1000, mounted)
  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
      .owned_by(1, 1)
      .named("Cruiser")
      .in_star_orbit(1, SystemCoordinates{100.0, 200.0})
      .with_cew(10, 1000)
      .with_crew(10, 10)
      .with_fuel(100.0)
      .build();

  // Create target ship #2 at distance ~14.14 (well within optical range)
  TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
      .owned_by(2, 1)
      .named("Freighter")
      .in_star_orbit(1, SystemCoordinates{110.0, 210.0})
      .with_armor(10)
      .with_crew(10, 0)
      .build();
}

void test_cew_happy_path_and_json() {
  TestContext ctx;
  setup_cew_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Fire CEW in ASCII mode
  ctx.assert_dispatch_success(g, {"cew", "#1", "#2"}, 1);
  test::expect_contains(g.out.str(), "CEW strength 10.");

  const auto* ship1 = ctx.em.peek_ship(1);
  test::expect_true(ship1 != nullptr);
  test::expect_eq(ship1->fuel(), 90.0);

  // 2. Fire CEW in JSON mode
  g.set_ui_mode(GB::presentation::UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"cew", "#1", "#2"}, 1);
  test::expect_contains(g.out.str(), "\"type\":\"fire_ship\"");
  test::expect_contains(g.out.str(), "\"cew_strength\":10");
  g.set_ui_mode(GB::presentation::UiMode::ASCII);

  ctx.verify_universe_invariants();
}

void test_cew_syntax_and_target_validation() {
  TestContext ctx;
  setup_cew_world(ctx);

  TestWorldBuilder(ctx).add_race("GuestAttacker", 100.0, /*guest=*/true,
                                 player_t{3});

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Guest rejection
  ctx.setup_game_obj(g, 3, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#2"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  // Switch back to Player 1
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 2. Too few arguments via dispatcher and direct command handler
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1"});
  g.out.str("");
  test::expect_false(GB::commands::cew({"cew", "#1"}, g));
  test::expect_contains(g.out.str(), "Syntax: 'cew <ship> <target>'.");

  // 3. Invalid target ship number
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#0"});
  test::expect_contains(g.out.str(), "Bad ship number.");

  // 4. Non-existent target ship
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#999"});

  // 5. Destroyed target ship
  const shipnum_t temp_target =
      TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
          .owned_by(2, 1)
          .named("Doomed")
          .in_star_orbit(1, SystemCoordinates{110.0, 210.0})
          .build();
  ctx.em.mutate_ship(temp_target, [&](Ship& s) { ctx.em.kill_ship(1, s); });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"cew", "#1", std::format("#{}", temp_target.value)});

  // 6. Self-targeting
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#1"});
  test::expect_contains(g.out.str(), "Get real.");

  ctx.verify_universe_invariants();
}

void test_cew_equipment_and_state_validation() {
  TestContext ctx;
  setup_cew_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  const UniverseCoordinates star_coords = ctx.em.peek_star(1)->coordinates();
  const UniverseCoordinates orig_s1_coords = ctx.em.peek_ship(1)->coordinates();
  const UniverseCoordinates orig_s2_coords = ctx.em.peek_ship(2)->coordinates();

  // 1. Inactive attacker (exercised via star-scope wildcard so
  // cew_single_ship's ShipIrradiated branch is reached directly)
  ctx.em.mutate_ship(1, [](Ship& s1) { s1.active() = false; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "*", "#2"});
  test::expect_contains(g.out.str(), "is irradiated and inactive.");
  ctx.em.mutate_ship(1, [](Ship& s1) { s1.active() = true; });

  // 2. Ship not equipped for CEWs
  ctx.em.mutate_ship(1, [](Ship& s1) { s1.cew() = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#2"});
  test::expect_contains(g.out.str(), "That ship is not equipped to fire CEWs.");
  ctx.em.mutate_ship(1, [](Ship& s1) { s1.cew() = 10; });

  // 3. No crystal mounted
  ctx.em.mutate_ship(1, [](Ship& s1) { s1.mounted() = false; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#2"});
  test::expect_contains(g.out.str(),
                        "You need to have a crystal mounted to fire CEWs.");
  ctx.em.mutate_ship(1, [](Ship& s1) { s1.mounted() = true; });

  // 4. Insufficient fuel
  ctx.em.mutate_ship(1, [](Ship& s1) { s1.admin_override_fuel(5.0); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#2"});
  test::expect_contains(g.out.str(), "You need 10 fuel to fire CEWs.");
  ctx.em.mutate_ship(1, [](Ship& s1) { s1.admin_override_fuel(100.0); });

  // 5. Landed attacker
  ctx.em.mutate_ship(
      1, [](Ship& s1) { s1.land_on_planet(1, 1, Coordinates{0, 0}); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#2"});
  test::expect_contains(
      g.out.str(),
      "CEWs cannot originate from or targeted to ships landed on planets.");
  ctx.em.mutate_ship(1, [&](Ship& s1) {
    s1.enter_star_orbit(1);
    s1.set_coordinates(orig_s1_coords);
  });

  // 6. Landed target
  ctx.em.mutate_ship(
      2, [](Ship& s2) { s2.land_on_planet(1, 1, Coordinates{0, 0}); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#2"});
  test::expect_contains(
      g.out.str(),
      "CEWs cannot originate from or targeted to ships landed on planets.");
  ctx.em.mutate_ship(2, [&](Ship& s2) {
    s2.enter_star_orbit(1);
    s2.set_coordinates(orig_s2_coords);
  });

  // 7. CEW power 1 produces initial_strength = 1 / 2 == 0 -> "No attack."
  ctx.em.mutate_ship(1, [](Ship& s1) { s1.cew() = 1; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#2"});
  test::expect_contains(g.out.str(), "No attack.");
  ctx.em.mutate_ship(1, [](Ship& s1) { s1.cew() = 10; });

  // 8. Target out of gun range -> "Illegal attack."
  ctx.em.mutate_ship(2, [&](Ship& s2) {
    s2.set_coordinates(star_coords + SystemCoordinates{50000.0, 50000.0});
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#2"});
  test::expect_contains(g.out.str(), "Illegal attack.");
  ctx.em.mutate_ship(2, [&](Ship& s2) { s2.set_coordinates(orig_s2_coords); });

  // 9. Insufficient Star AP
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"cew", "#1", "#2"});
  test::expect_contains(g.out.str(), "You don't have 1 action points there.");

  ctx.verify_universe_invariants();
}

void test_cew_overload_explosion() {
  TestContext ctx;
  setup_cew_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // Give Ship #1 0 tech and high CEW power so check_overload deterministically
  // explodes the crystal and kills the ship.
  ctx.em.mutate_ship(1, [](Ship& s1) {
    s1.tech() = 0.0;
    s1.cew() = 10000;
    s1.admin_override_max_fuel(30000.0);
    s1.admin_override_fuel(30000.0);
  });

  ctx.assert_dispatch_success(g, {"cew", "#1", "#2"}, 1);
  test::expect_contains(g.out.str(), "No attack.");
  test::expect_throws<EntityNotFoundError>([&]() { ctx.em.peek_ship(1); });

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_cew_happy_path_and_json();
  test_cew_syntax_and_target_validation();
  test_cew_equipment_and_state_validation();
  test_cew_overload_explosion();
  std::println("All cew tests passed!");
  return 0;
}
