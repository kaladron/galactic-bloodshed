// SPDX-License-Identifier: Apache-2.0

/// \file fire_test.cc
/// \brief Unit tests for fire command and ship-to-ship weapon mechanics.

import commands;
import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Create attacker ship #1 - armed with guns
  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
      .owned_by(1, 1)
      .named("Battleship")
      .in_star_orbit(1, SystemCoordinates{100.0, 200.0})
      .with_guns(guntype_t::LIGHT, 10)
      .with_destruct(100)
      .with_crew(10, 10)
      .with_fuel(1000.0)
      .build();

  // Create target ship #2
  TestShipBuilder(ctx.em, ShipType::STYPE_CARGO)
      .owned_by(2, 1)
      .named("Target")
      .in_star_orbit(1, SystemCoordinates{110.0, 210.0})
      .with_armor(10)
      .with_crew(10, 0)
      .build();
}

void test_fire_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Execute fire command: Ship #1 attacks Ship #2 with strength 10
  ctx.assert_dispatch_success(g, {"fire", "#1", "#2", "10"}, 1);

  const auto* ship1 = ctx.em.peek_ship(1);
  test::expect_true(ship1 != nullptr);
  test::expect_eq(ship1->number(), 1);
  test::expect_lt(ship1->destruct(), 100);

  const auto* ship2 = ctx.em.peek_ship(2);
  test::expect_true(ship2 != nullptr);
  test::expect_eq(ship2->number(), 2);

  // 2. Execute fire command in JSON mode and with clamped gun strength
  g.set_ui_mode(GB::presentation::UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"fire", "#1", "#2", "999"}, 1);
  test::expect_contains(g.out.str(), "\"type\":\"fire_ship\"");
  g.set_ui_mode(GB::presentation::UiMode::ASCII);

  // 3. Clamped gun strength in ASCII mode
  g.out.str("");
  ctx.assert_dispatch_success(g, {"fire", "#1", "#2", "999"}, 1);
  test::expect_contains(g.out.str(), "Guns set to");

  ctx.verify_universe_invariants();
}

void test_fire_universe_ap_and_god_exemption() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  // 1. Firing from Universe command scope with ships in Star 1 orbit succeeds
  // and deducts 1 Star AP from Star 1.
  const ap_t star_ap_before = ctx.em.peek_star(1)->AP(1);
  bool ok = ctx.dispatch(g, {"fire", "#1", "#2", "10"});
  test::expect_true(ok);
  test::expect_eq(ctx.em.peek_star(1)->AP(1), star_ap_before - 1);

  // 2. God player with 0 Star AP can still fire without deducting AP.
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });
  g.set_god(true);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"fire", "#1", "#2", "1"}, 0);
  g.set_god(false);

  // 3. A ship in deep space (LEVEL_UNIV) checks Universe AP first, and is
  // rejected as an illegal attack by shoot_ship_to_ship without deducting AP.
  ctx.em.mutate_ship(1, [](Ship& s1) { s1.enter_deep_space(); });
  ctx.em.mutate_universe([](universe_struct& u) { u.set_AP(1, 0); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "10"});
  test::expect_contains(g.out.str(), "You need 1 universe action points.");

  ctx.em.mutate_universe([](universe_struct& u) { u.set_AP(1, 50); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "10"});
  test::expect_contains(g.out.str(), "Illegal attack.");
  test::expect_eq(ctx.em.peek_universe()->get_AP(1), 50);

  ctx.verify_universe_invariants();
}

void test_fire_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "10"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_fire_role_and_guest_rejections() {
  TestContext ctx;
  setup_test_world(ctx);

  TestWorldBuilder(ctx).add_race("GuestAttacker", 100.0, /*guest=*/true,
                                 player_t{3});

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  ctx.setup_game_obj(g, 3, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "10"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  ctx.verify_universe_invariants();
}

void test_fire_domain_errors_and_surface_geometry() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Min args check (< 3 args) via dispatch and direct handler call
  ctx.assert_dispatch_rejected(g, {"fire", "#1"});
  test::expect_contains(g.out.str(),
                        "Syntax: fire <ship> <target> [<strength>]");
  g.out.str("");
  test::expect_false(GB::commands::fire({"fire", "#1"}, g));
  test::expect_contains(g.out.str(),
                        "Syntax: 'fire <ship> <target> [<strength>]'.");

  // 2. Bad target ship number, non-numeric strength, zero strength, missing
  // target
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "invalid"});
  test::expect_contains(g.out.str(), "Bad ship number.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "notanumber"});
  test::expect_contains(g.out.str(), "No attack.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#2", "0"});
  test::expect_contains(g.out.str(), "No attack.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#999", "10"});

  // 3. Target self
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fire", "#1", "#1", "10"});
  test::expect_contains(g.out.str(), "Get real.");

  // 4. Inactive ship (tested via star-scope wildcard so fire_single_ship's
  // ShipIrradiated branch is exercised directly)
  ctx.em.mutate_ship(1, [](Ship& s) { s.active() = false; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fire", "*", "#2", "10"});
  test::expect_contains(g.out.str(), "is irradiated and inactive.");
  ctx.em.mutate_ship(1, [](Ship& s) { s.active() = true; });

  // 5. AFV and surface combat geometry checks
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_pnum(1);
  const auto afv_id = TestShipBuilder(ctx.em, ShipType::OTYPE_AFV)
                          .owned_by(1, 1)
                          .in_planet_orbit(1, 1)
                          .with_guns(guntype_t::LIGHT, 5)
                          .with_destruct(20)
                          .with_crew(5, 5)
                          .build();
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"fire", std::format("#{}", afv_id.value), "#2"});
  test::expect_contains(g.out.str(), "isn't landed on a planet!");

  // Land AFV at (0, 0) and target #2 in orbit -> rejected
  ctx.em.mutate_ship(afv_id, [](Ship& s) {
    s.land_on_planet();
    s.set_land_coords({0, 0});
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"fire", std::format("#{}", afv_id.value), "#2"});
  test::expect_contains(g.out.str(), "isn't landed on a planet!");

  // Land target #2 on a DIFFERENT planet (Star 2, Planet 1) -> rejected
  ctx.em.mutate_ship(2, [](Ship& s) {
    s.enter_planet_orbit(2, 1);
    s.land_on_planet();
    s.set_land_coords({0, 0});
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"fire", std::format("#{}", afv_id.value), "#2"});
  test::expect_contains(
      g.out.str(),
      "Landed ships can only attack other landed ships if they are on the same "
      "planet!");

  // Land target #2 on same planet, non-adjacent sector (5, 5) -> rejected
  ctx.em.mutate_ship(2, [](Ship& s) {
    s.enter_planet_orbit(1, 1);
    s.land_on_planet();
    s.set_land_coords({5, 5});
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(
      g, {"fire", std::format("#{}", afv_id.value), "#2"});
  test::expect_contains(g.out.str(), "You are not adjacent to your target!");

  // Land target #2 on adjacent sector (0, 1) -> succeeds!
  ctx.em.mutate_ship(2, [](Ship& s) { s.set_land_coords({0, 1}); });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"fire", std::format("#{}", afv_id.value), "#2"}, 1);

  // 6. Target self-retaliation (protect().retaliate) and laser fire clamping
  g.set_level(ScopeLevel::LEVEL_STAR);
  ctx.em.mutate_ship(1, [](Ship& s) {
    s.laser() = true;
    s.fire_laser() = 10;
    s.mounted() = true;
  });
  ctx.em.mutate_ship(2, [](Ship& s) {
    s.launch_to_orbit(ScopeLevel::LEVEL_STAR);
    s.set_primary_battery(5, guntype_t::LIGHT);
    s.destruct() = 50;
    s.protect().retaliate = true;
  });
  g.out.str("");
  ctx.assert_dispatch_success(g, {"fire", "#1", "#2", "999"}, 1);
  test::expect_contains(g.out.str(), "Laser strength set to");
  test::expect_lt(ctx.em.peek_ship(2)->destruct(), 50);

  ctx.verify_universe_invariants();
}

void test_protecting_ship_retaliation() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create an escort ship protecting Target (Ship #2)
  TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
      .owned_by(2, 1)
      .named("Escort")
      .in_star_orbit(1, SystemCoordinates{110.0, 210.0})
      .with_guns(guntype_t::LIGHT, 1)
      .with_destruct(100)
      .with_crew(10, 10)
      .with_fuel(1000.0)
      .build();

  ctx.em.mutate_ship(3, [](Ship& escort) {
    escort.protect().on = true;
    escort.protect().ship = 2;
  });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  // 1. Live protecting ship retaliates when target takes damage
  ctx.assert_dispatch_success(g, {"fire", "#1", "#2", "10"}, 1);

  const auto* escort = ctx.em.peek_ship(3);
  test::expect_true(escort != nullptr);
  test::expect_lt(escort->destruct(), 100);

  const auto* attacker = ctx.em.peek_ship(1);
  test::expect_true(attacker != nullptr);
  test::expect_true(attacker->alive());
  test::expect_gt(attacker->damage(), 0);

  // 2. Dead protecting ship does NOT retaliate
  ctx.em.mutate_ship(3, [&](Ship& s) { ctx.em.kill_ship(1, s); });
  const auto attacker_damage_before = ctx.em.peek_ship(1)->damage();

  ctx.assert_dispatch_success(g, {"fire", "#1", "#2", "10"}, 1);
  test::expect_eq(ctx.em.peek_ship(1)->damage(), attacker_damage_before);

  ctx.verify_universe_invariants();
}

void test_fire_crystal_overload_burnout_and_explosion() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(1);

  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 20; });

  // 1. Direct check_overload early-return MC/DC branches:
  ctx.em.mutate_ship(1, [&](Ship& s) {
    s.laser() = false;
    s.fire_laser() = 0;
    auto [str1, ev1] = check_overload(ctx.em, s, 0, 10);
    test::expect_false(ev1.has_value());
    test::expect_eq(str1, 10u);

    s.laser() = true;
    s.fire_laser() = 0;
    auto [str2, ev2] = check_overload(ctx.em, s, 0, 10);
    test::expect_false(ev2.has_value());
    test::expect_eq(str2, 10u);
  });

  // 2. Non-lethal crystal burnout during laser fire:
  shipnum_t burnout_ship =
      TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
          .owned_by(1, 1)
          .named("BurnoutLaser")
          .in_star_orbit(1, SystemCoordinates{100.0, 200.0})
          .with_crew(10, 10)
          .with_fuel(1000.0)
          .build();
  ctx.em.mutate_ship(burnout_ship, [](Ship& s) {
    s.tech() = 2.0;
    s.laser() = true;
    s.fire_laser() = 1;
    s.mounted() = true;
  });

  bool burned_out = false;
  for (int attempt = 0; attempt < 40 && !burned_out; ++attempt) {
    ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 10; });
    g.out.str("");
    ctx.assert_dispatch_success(
        g, {"fire", std::format("#{}", burnout_ship.value), "#2", "1"}, 1);
    const auto* s = ctx.em.peek_ship(burnout_ship);
    test::expect_true(s != nullptr && s->alive());
    if (!s->mounted()) {
      burned_out = true;
      test::expect_eq(s->fire_laser(), 0u);
      test::expect_contains(g.out.str(), "No attack.");
      test::expect_eq(ctx.em.peek_star(1)->AP(1), 9);
    }
  }
  test::expect_true(burned_out);

  // 3. Lethal crystal explosion during laser fire at LEVEL_STAR:
  shipnum_t star_explode_ship =
      TestShipBuilder(ctx.em, ShipType::STYPE_BATTLE)
          .owned_by(1, 1)
          .named("StarExplodeLaser")
          .in_star_orbit(1, SystemCoordinates{100.0, 200.0})
          .with_crew(10, 10)
          .with_fuel(30000.0)
          .build();
  ctx.em.mutate_ship(star_explode_ship, [](Ship& s) {
    s.tech() = 0.0;
    s.laser() = true;
    s.fire_laser() = 10000;
    s.mounted() = true;
  });
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 5; });
  g.out.str("");
  ctx.assert_dispatch_success(
      g, {"fire", std::format("#{}", star_explode_ship.value), "#2", "10000"},
      1);
  test::expect_contains(g.out.str(), "No attack.");
  test::expect_eq(ctx.em.peek_star(1)->AP(1), 4);
  test::expect_throws<EntityNotFoundError>(
      [&]() { ctx.em.peek_ship(star_explode_ship); });

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_fire_happy_paths();
  test_fire_universe_ap_and_god_exemption();
  test_fire_insufficient_ap();
  test_fire_role_and_guest_rejections();
  test_fire_domain_errors_and_surface_geometry();
  test_protecting_ship_retaliation();
  test_fire_crystal_overload_burnout_and_explosion();

  std::println(std::cout, "✓ fire_test passed!");
  return 0;
}
