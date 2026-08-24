// SPDX-License-Identifier: Apache-2.0

/// \file enslave_test.cc
/// \brief Unit tests for enslave command

import commands;
import dallib;
import gblib;
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
  {
    auto planet_handle = ctx.em.get_planet(0, 0);
    planet_handle->info(player_t{1}).numsectsowned = 5;
    planet_handle->info(player_t{2}).popn = 1000;
    planet_handle->info(player_t{2}).numsectsowned = 5;
    planet_handle->info(player_t{1}).destruct = 1000;
    planet_handle->info(player_t{2}).destruct = 100;
    planet_handle->slaved_to() = 0;
    planet_handle->ships() = 1;
  }

  // Create OAP ship in planet orbit
  TestShipBuilder(ctx.em, ShipType::STYPE_OAP)
      .owned_by(1, 0)
      .named("OAP")
      .in_planet_orbit(0, 0, 0.0, 0.0)
      .with_destruct(500)
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

  ctx.assert_dispatch_success(g, {"enslave", "1"});

  // Verify planet was enslaved
  ctx.em.clear_cache();
  const auto* saved_planet = ctx.em.peek_planet(0, 0);
  test::expect_true(saved_planet != nullptr);
  test::expect_eq(saved_planet->slaved_to(), 1);

  ctx.verify_universe_invariants();
}

void test_enslave_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set AP to 0
  {
    auto star_handle = ctx.em.get_star(0);
    star_handle->AP(1) = 0;
  }

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
  {
    auto ship_handle = ctx.em.get_ship(1);
    ship_handle->type() = ShipType::STYPE_CARGO;
  }
  ctx.assert_dispatch_rejected(g, {"enslave", "1"});
  test::expect_contains(g.out.str(), "not an Ob Asst Pltfrm");

  ctx.verify_universe_invariants();
}

}  // namespace

int main() {
  test_enslave_happy_path();
  test_enslave_insufficient_ap();
  test_enslave_role_rejection();
  test_enslave_domain_errors();

  std::println(std::cout, "✓ enslave_test passed!");
  return 0;
}
