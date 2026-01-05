// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

int main() {
  // Create in-memory database
  TestContext ctx;

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.Guest = false;
  race.governor[0].active = true;
  race.governor[0].toggle.highlight = true;

  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create star system
  star_struct ss{};
  ss.star_id = 0;
  ss.pnames.emplace_back("TestPlanet");
  ss.AP[0] = 100;  // Player 1 APs
  StarRepository star_repo(store);
  star_repo.save(ss);

  // Create planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;

  PlanetRepository planet_repo(store);
  planet_repo.save(planet);

  // Create and initialize sector map
  {
    SectorMap smap(planet, true);
    smap.get(5, 5).set_condition(SectorType::SEC_LAND);
    smap.get(5, 5).set_popn(100);
    smap.get(5, 5).set_owner(2);  // Owned by race 2
    smap.get(5, 5).set_troops(10);

    SectorRepository smap_repo(store);
    smap_repo.save_map(smap);
  }

  // Create attacker ship in orbit
  ship_struct attacker{};
  attacker.number = 1;
  attacker.owner = 1;
  attacker.governor = 0;
  attacker.alive = true;
  attacker.on = true;
  attacker.type = ShipType::STYPE_BATTLE;
  attacker.guns = 1;  // Light guns
  attacker.destruct = 100;
  attacker.fuel = 1000.0;
  attacker.whatorbits = ScopeLevel::LEVEL_PLAN;
  attacker.storbits = 0;
  attacker.pnumorbits = 0;
  attacker.xpos = 100.0;
  attacker.ypos = 200.0;
  attacker.mass = 100.0;
  attacker.build_cost = 100;

  auto attacker_handle = ctx.em.create_ship(attacker);
  attacker_handle.save();

  // Create target race
  Race target_race{};
  target_race.Playernum = 2;
  target_race.Guest = false;
  target_race.governor[0].active = true;
  races.save(target_race);

  // Setup GameObj
  auto* registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Execute bombard command on sector 5,5 with strength 10
  command_t argv = {"bombard", "#1", "5,5", "10"};
  GB::commands::bombard(argv, g);

  // Verify ship and planet still exist in database (persisted via
  // EntityManager)
  const auto* ship = ctx.em.peek_ship(1);
  assert(ship);
  assert(ship->number() == 1);

  const auto* planet_after = ctx.em.peek_planet(0, 0);
  assert(planet_after);

  // Verify sector map persisted
  const auto* smap_after = ctx.em.peek_sectormap(0, 0);
  assert(smap_after);

  // Test passed - command executed and data persisted via RAII
  std::println("bombard_test.cc: All assertions passed!");
  return 0;
}
