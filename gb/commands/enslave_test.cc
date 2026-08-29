// SPDX-License-Identifier: Apache-2.0

/// \file enslave_test.cc
/// \brief Unit tests for enslave command

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

namespace {

void setup_test_world(TestContext& ctx) {
  TestWorldBuilder(ctx)
      .add_race("Enslavers", 100.0, false, player_t{1})
      .add_race("Victims", 100.0, false, player_t{2})
      .add_star("Test Star", 100, starnum_t{0})
      .add_planet(0, PlanetType::EARTH);

  // Setup planet info
  ctx.em.mutate_planet(0, 0, [](Planet& planet) {
    planet.info(player_t{1}).numsectsowned = 5;
    planet.info(player_t{2}).popn = 1000;
    planet.info(player_t{2}).numsectsowned = 5;
    planet.info(player_t{1}).destruct = 1000;
    planet.info(player_t{2}).destruct = 100;
    planet.slaved_to() = 0;
    planet.ships() = 1;
  });

  // Create OAP ship in planet orbit
  TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
      .owned_by(1, 0)
      .named("Observer")
      .in_planet_orbit(0, 0)
      .build();
}

void test_enslave_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(0);
  g.set_pnum(0);

  // Enslave victim race (player 2)
  ctx.assert_dispatch_success(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "Enslavement successful");

  // Verify planet is slaved to player 1
  const auto* planet = ctx.em.peek_planet(0, 0);
  test::expect_true(planet != nullptr);
  test::expect_eq(planet->slaved_to(), 1);

  ctx.verify_universe_invariants();
}

void test_enslave_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set AP to 0
  ctx.em.mutate_star(0, [](Star& s) { s.AP(1) = 0; });

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "action points");

  ctx.verify_universe_invariants();
}

void test_enslave_role_rejection() {
  TestContext ctx;
  setup_test_world(ctx);

  // Create guest race
  Race guest{};
  guest.Playernum = 3;
  guest.name = "GuestEnslaver";
  guest.Guest = true;
  guest.governor[0].active = true;
  {
    JsonStore store(ctx.db);
    RaceRepository races(store);
    races.save(guest);
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 3, 0);
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
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(0);
  g.set_pnum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"enslave"});
  test::expect_contains(g.out.str(), "Syntax: enslave <ship>");

  // 2. Ship not an OAP
  ctx.em.mutate_ship(1, [](Ship& s) { s.type() = ShipType::STYPE_CARGO; });
  ctx.assert_dispatch_rejected(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "not an Ob Asst Pltfrm");

  ctx.verify_universe_invariants();
}

void test_enslave_maxplayers_boundary() {
  TestContext ctx;
  TestWorldBuilder(ctx)
      .add_race("Enslavers", 100.0, false, player_t{1})
      .add_race("VictimMax", 100.0, false, player_t{MAXPLAYERS})
      .add_star("Test Star", 100, starnum_t{0})
      .add_planet(0, PlanetType::EARTH);

  ctx.em.mutate_planet(0, 0, [](Planet& planet) {
    planet.info(player_t{1}).numsectsowned = 5;
    planet.info(player_t{MAXPLAYERS}).popn = 1000;
    planet.info(player_t{MAXPLAYERS}).numsectsowned = 5;
    planet.info(player_t{1}).destruct = 1000;
    planet.info(player_t{MAXPLAYERS}).destruct = 100;
    planet.slaved_to() = 0;
    planet.ships() = 1;
  });

  TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
      .owned_by(1, 0)
      .named("Observer")
      .in_planet_orbit(0, 0)
      .build();

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(0);
  g.set_pnum(0);

  ctx.assert_dispatch_success(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "Enslavement successful");

  const auto* planet = ctx.em.peek_planet(0, 0);
  test::expect_true(planet != nullptr);
  test::expect_eq(planet->slaved_to(), 1);

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_enslave_happy_path();
  test_enslave_insufficient_ap();
  test_enslave_role_rejection();
  test_enslave_domain_errors();
  test_enslave_maxplayers_boundary();

  std::println(std::cout, "✓ enslave_test passed!");
  return 0;
}
