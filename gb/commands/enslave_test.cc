// SPDX-License-Identifier: Apache-2.0

/// \file enslave_test.cc
/// \brief Unit tests for enslave command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "Enslavers";
  race.Guest = false;
  race.governor[0].active = true;
  race.governor[0].toggle.highlight = true;

  RaceRepository races(store);
  races.save(race);

  // Create enemy race
  Race enemy{};
  enemy.Playernum = 2;
  enemy.name = "Victims";
  enemy.Guest = false;
  enemy.governor[0].active = true;
  races.save(enemy);

  // Create test star
  star_struct star{};
  star.star_id = 0;
  star.name = "Test Star";
  star.AP[0] = 100;
  star.pnames.emplace_back("Test Planet");

  StarRepository stars(store);
  stars.save(star);

  // Create test planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.info(player_t{1}).numsectsowned = 5;
  planet.info(player_t{2}).popn = 1000;
  planet.info(player_t{2}).numsectsowned = 5;
  planet.info(player_t{1}).destruct = 1000;
  planet.info(player_t{2}).destruct = 100;
  planet.slaved_to() = 0;
  planet.ships() = 1;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create OAP ship
  Ship oap{};
  oap.number() = 1;
  oap.owner() = 1;
  oap.governor() = 0;
  oap.alive() = true;
  oap.active() = true;
  oap.type() = ShipType::STYPE_OAP;
  oap.whatorbits() = ScopeLevel::LEVEL_PLAN;
  oap.storbits() = 0;
  oap.pnumorbits() = 0;
  oap.destruct() = 500;
  oap.tech() = 100.0;
  oap.size() = 100;
  oap.build_cost() = 100;

  ShipRepository ships(store);
  ships.save(oap);
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
  assert(saved_planet);
  assert(saved_planet->slaved_to() == 1);
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
  assert(g.out.str().contains("action points"));
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
  assert(g.out.str().contains("Guest races cannot use this command."));
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
  assert(g.out.str().contains("Syntax: enslave <ship>"));

  // 2. Ship not an OAP
  {
    auto ship_handle = ctx.em.get_ship(1);
    ship_handle->type() = ShipType::STYPE_CARGO;
  }
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"enslave", "1"});
  assert(g.out.str().contains("not an Ob Asst Pltfrm"));
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
