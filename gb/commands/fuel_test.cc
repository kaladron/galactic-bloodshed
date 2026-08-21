// SPDX-License-Identifier: Apache-2.0

/// \file fuel_test.cc
/// \brief Unit tests for fuel (proj_fuel) command

import commands;
import dallib;
import gblib;
import test;
import std;

#include <cassert>

namespace {

void setup_test_world(TestContext& ctx) {
  JsonStore store(ctx.db);

  universe_struct us{};
  us.id = 1;
  us.numstars = 2;
  UniverseRepository universe_repo(store);
  universe_repo.save(us);

  Race race{};
  race.Playernum = 1;
  race.name = "Stargazers";
  race.Guest = false;
  race.governor[0].active = true;
  race.mass = 1.0;

  RaceRepository races(store);
  races.save(race);

  star_struct ss0{};
  ss0.star_id = 0;
  ss0.name = "OriginStar";
  ss0.xpos = 0.0;
  ss0.ypos = 0.0;
  ss0.explored = (1ULL << 1);
  ss0.pnames.emplace_back("OriginPlanet");

  star_struct ss1{};
  ss1.star_id = 1;
  ss1.name = "DestStar";
  ss1.xpos = 100.0;
  ss1.ypos = 100.0;
  ss1.explored = (1ULL << 1);
  ss1.pnames.emplace_back("DestPlanet");

  StarRepository stars(store);
  stars.save(ss0);
  stars.save(ss1);

  planet_struct ps0{};
  ps0.star_id = 0;
  ps0.planet_order = 0;
  ps0.type = PlanetType::EARTH;
  planet_struct ps1{};
  ps1.star_id = 1;
  ps1.planet_order = 0;
  ps1.type = PlanetType::EARTH;

  PlanetRepository planets(store);
  planets.save(ps0);
  planets.save(ps1);

  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_CRUISER;
  ship.name() = "Explorer";
  ship.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship.storbits() = 0;
  ship.xpos() = 0.0;
  ship.ypos() = 0.0;
  ship.speed() = 2;
  ship.max_speed() = 5;
  ship.fuel() = 100.0;
  ship.max_fuel() = 500.0;
  ship.mass() = 100.0;

  ShipRepository ships(store);
  ships.save(ship);
}

void test_fuel_happy_path() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  ctx.assert_dispatch_success(g, {"fuel", "#1", "/DestStar"});
  assert(g.out.str().contains("FUEL ESTIMATES"));
}

void test_fuel_domain_errors() {
  TestContext ctx;
  setup_test_world(ctx);

  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g, 1, 0);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  // 1. Min args check (< 2 args)
  ctx.assert_dispatch_rejected(g, {"fuel"});
  assert(g.out.str().contains("Syntax: fuel <#ship> [<destination>]"));

  // 2. Non-existent ship
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fuel", "#999", "/DestStar"});
  assert(g.out.str().contains("rst: no such ship #999"));

  // 3. Bad argument format (not starting with #)
  g.out.str("");
  ctx.assert_dispatch_rejected(g, {"fuel", "1", "/DestStar"});
  assert(g.out.str().contains("Invalid first option"));
}

}  // namespace

int main() {
  test_fuel_happy_path();
  test_fuel_domain_errors();

  std::println(std::cout, "✓ fuel_test passed!");
  return 0;
}
