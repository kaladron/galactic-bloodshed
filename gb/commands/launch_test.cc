// SPDX-License-Identifier: Apache-2.0

/// \file launch_test.cc
/// \brief Unit tests for launch and undock commands

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  // Initialize in-memory database
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.governor[0].money = 10000;
  race.governor[0].toggle.highlight = true;
  RaceRepository races(store);
  races.save(race);

  // Create test star
  star_struct ss{};
  ss.star_id = 1;
  ss.pnames.emplace_back(
      "TestPlanet");  // numplanets is derived from pnames.size()
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.AP[0] = 100;
  Star star(ss);
  star.set_name("TestStar");
  StarRepository stars(store);
  stars.save(star);

  // Create test planet
  planet_struct ps{};
  ps.star_id = 1;
  ps.planet_order = 0;
  ps.Maxx = 10;
  ps.Maxy = 10;
  ps.info[0].numsectsowned = 5;
  ps.xpos = 10.0;
  ps.ypos = 20.0;
  ps.explored = 0;
  Planet planet(ps);
  PlanetRepository planets(store);
  planets.save(planet);

  // Create test ship that's landed on the planet
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;   // CRITICAL: Ship must be alive
  ship.active() = true;  // CRITICAL: Ship must be active
  ship.type() = ShipType::STYPE_SHUTTLE;
  ship.max_speed() = 5;  // CRITICAL: Ship needs speed_rating to be launchable
  ship.xpos() = 110.0;
  ship.ypos() = 220.0;
  ship.set_land_coords({5, 5});
  ship.fuel() = 1000.0;
  ship.mass() = 100.0;
  ship.docked() = 1;
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.storbits() = 1;
  ship.pnumorbits() = 0;
  ship.whatdest() = ScopeLevel::LEVEL_PLAN;
  ship.deststar() = 1;
  ship.destpnum() = 0;
  ShipRepository ships(store);
  ships.save(ship);
}

void test_launch_happy_paths() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // 1. Launch landed ship from planet (costs 1 Star AP)
  ctx.assert_dispatch_success(g, {"launch", "#1"}, 1);
  assert(g.out.str().contains("launched from planet"));

  // Verify ship is no longer docked and has fuel consumed
  const auto* launched_ship = ctx.em.peek_ship(1);
  assert(launched_ship);
  assert(launched_ship->docked() == 0);
  assert(launched_ship->whatdest() == ScopeLevel::LEVEL_UNIV);
  assert(launched_ship->fuel() < 1000.0);  // Fuel consumed

  // Verify planet is now explored
  const auto* explored_planet = ctx.em.peek_planet(1, 0);
  assert(explored_planet);
  assert(explored_planet->explored() == 1);

  // 2. Undock alias dispatch
  g.out.str("");
  // Re-dock ship to another ship to test undock
  {
    auto s1 = ctx.em.get_ship(1);
    s1->docked() = 1;
    s1->whatdest() = ScopeLevel::LEVEL_SHIP;
    s1->destshipno() = 1;  // Mock target
  }
  ctx.assert_dispatch_success(g, {"undock", "#1"}, 0);
  assert(g.out.str().contains("undocked"));
}

void test_launch_insufficient_ap() {
  TestContext ctx;
  setup_test_world(ctx);

  // Set Star AP to 0
  {
    auto star_handle = ctx.em.get_star(1);
    star_handle->AP(1) = 0;
  }

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  ctx.assert_dispatch_rejected(g, {"launch", "#1"});
  assert(g.out.str().contains("action points"));
}

void test_launch_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"launch"});
  assert(g.out.str().contains("Syntax: launch <ship>"));

  // 2. Launch non-docked/non-landed ship
  {
    auto s1 = ctx.em.get_ship(1);
    s1->docked() = 0;
    s1->whatorbits() = ScopeLevel::LEVEL_PLAN;
    s1->whatdest() = ScopeLevel::LEVEL_UNIV;
  }
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"launch", "#1"});
  assert(g.out.str().contains("is not landed or docked"));
}

}  // namespace

int main() {
  test_launch_happy_paths();
  test_launch_insufficient_ap();
  test_launch_domain_errors();

  std::println(std::cout, "✓ launch_test passed!");
  return 0;
}
