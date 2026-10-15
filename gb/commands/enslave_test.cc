// SPDX-License-Identifier: Apache-2.0

/// \file enslave_test.cc
/// \brief Unit tests for enslave command

import commands;
import gb.entities;
import gb.presentation;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  ctx.with_standard_universe();

  // Setup planet info
  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.info(player_t{1}).numsectsowned = 5;
    planet.info(player_t{2}).popn = 1000;
    planet.info(player_t{2}).numsectsowned = 5;
    planet.info(player_t{1}).destruct = 1000;
    planet.info(player_t{2}).destruct = 10;
    planet.free_slaves();
  });

  // Create OAP ship in planet orbit with destruct > 0
  TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
      .owned_by(1, 1)
      .named("Observer")
      .in_planet_orbit(1, 1)
      .with_destruct(100)
      .build();
}

void test_enslave_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(1);
  g.set_pnum(1);

  // Enslave victim race (player 2)
  ctx.assert_dispatch_success(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "Enslavement successful");
  test::expect_contains(g.out.str(), "0.1% the population");

  // Verify planet is slaved to player 1
  const auto* planet = ctx.em.peek_planet(1, 1);
  test::expect_true(planet != nullptr);
  test::expect_eq(planet->slaved_to(), player_t{1});

  ctx.verify_universe_invariants();
}

void test_enslave_repulsed_and_json() {
  TestContext ctx;
  setup_test_world(ctx);

  // Give defender (player 2) an orbiting ship with high destruct and an
  // inactive ship
  TestShipBuilder(ctx.em, ShipType::STYPE_DREADNT)
      .owned_by(2, 1)
      .in_planet_orbit(1, 1)
      .with_destruct(500)
      .build();
  TestShipBuilder(ctx.em, ShipType::STYPE_DREADNT)
      .owned_by(2, 1)
      .in_planet_orbit(1, 1)
      .with_active(false)
      .with_destruct(500)
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_success(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "Enslavement repulsed.");
  test::expect_eq(ctx.em.peek_planet(1, 1)->slaved_to(), std::nullopt);

  // Now boost attacker destruct and test JSON mode
  ctx.em.mutate_ship(1, [](Ship& s) { s.destruct() = 1000; });
  g.set_ui_mode(GB::presentation::UiMode::JSON);
  g.out.str("");
  ctx.assert_dispatch_success(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), R"("type":"enslave")");
  test::expect_contains(g.out.str(), R"("enslaved":true)");

  ctx.verify_universe_invariants();
}

void test_enslave_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set AP to 0
  ctx.em.mutate_star(1, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_rejected(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_enslave_role_rejection() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create guest race
  TestWorldBuilder(ctx).add_race("GuestEnslaver", 100.0, true, player_t{3});

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 3, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);

  ctx.assert_dispatch_rejected(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "Guest races cannot use this command.");

  ctx.verify_universe_invariants();
}

void test_enslave_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(1);
  g.set_pnum(1);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"enslave"});
  test::expect_contains(g.out.str(), "Syntax: enslave <ship>");

  // 2. Invalid ship string & non-existent ship
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"enslave", "invalid"});
  test::expect_contains(g.out.str(), "Specify a valid ship number.");

  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"enslave", "999"});
  test::expect_contains(g.out.str(), "Ship not found.");

  // 3. No aliens on planet
  ctx.em.mutate_planet(1, 1, [](Planet& p) {
    p.info(player_t{2}).numsectsowned = 0;
    p.info(player_t{3}).numsectsowned = 0;
    p.info(player_t{4}).numsectsowned = 0;
  });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"enslave", "1"});
  test::expect_contains(g.out.str(),
                        "There is no one else on this planet to enslave!");

  // 4. No garrison on planet
  ctx.em.mutate_planet(
      1, 1, [](Planet& p) { p.info(player_t{1}).numsectsowned = 0; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"enslave", "1"});
  test::expect_contains(g.out.str(),
                        "You don't have a garrison on the planet.");

  // 5. Ship not orbiting a planet
  ctx.em.mutate_ship(1, [](Ship& s) { s.enter_star_orbit(1); });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "doesn't orbit a planet.");

  // 6. Ship not an OAP
  ctx.em.mutate_ship(1, [](Ship& s) { s.type() = ShipType::STYPE_CARGO; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "not an Ob Asst Pltfrm");

  // 7. Ship owned by another player
  ctx.em.mutate_ship(1, [](Ship& s) { s.owner() = 2; });
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"enslave", "1"});

  ctx.verify_universe_invariants();
}

void test_enslave_maxplayers_boundary() {
  TestContext ctx;
  TestWorldBuilder(ctx)
      .add_race("Enslavers", 100.0, false, player_t{1})
      .add_race("VictimMax", 100.0, false, player_t{100})
      .add_star("Test Star", 100, starnum_t{1})
      .add_planet(1, PlanetType::EARTH);

  ctx.em.mutate_planet(1, 1, [](Planet& planet) {
    planet.info(player_t{1}).numsectsowned = 5;
    planet.info(player_t{100}).popn = 1000;
    planet.info(player_t{100}).numsectsowned = 5;
    planet.info(player_t{1}).destruct = 1000;
    planet.info(player_t{100}).destruct = 10;
    planet.free_slaves();
  });

  TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
      .owned_by(1, 1)
      .named("Observer")
      .in_planet_orbit(1, 1)
      .with_destruct(100)
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(1);
  g.set_pnum(1);

  ctx.assert_dispatch_success(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "Enslavement successful");

  const auto* planet = ctx.em.peek_planet(1, 1);
  test::expect_true(planet != nullptr);
  test::expect_eq(planet->slaved_to(), player_t{1});

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_enslave_happy_path();
  test_enslave_repulsed_and_json();
  test_enslave_insufficient_ap();
  test_enslave_role_rejection();
  test_enslave_domain_errors();
  test_enslave_maxplayers_boundary();

  std::println(std::cout, "✓ enslave_test passed!");
  return 0;
}
