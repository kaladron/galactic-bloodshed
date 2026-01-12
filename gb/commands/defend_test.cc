// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

int main() {
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "Testers";
  race.Guest = false;

  RaceRepository races(store);
  races.save(race);

  // Create enemy race
  Race enemy{};
  enemy.Playernum = 2;
  enemy.name = "Enemies";
  enemy.Guest = false;
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
  planet.info(player_t{1}).numsectsowned = 1;
  planet.info(player_t{1}).guns = 50;
  planet.info(player_t{1}).destruct = 100;
  planet.xpos() = 0.0;
  planet.ypos() = 0.0;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create attacking ship
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 2;
  ship.alive() = true;
  ship.type() = ShipType::OTYPE_FACTORY;
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.storbits() = 0;
  ship.pnumorbits() = 0;
  ship.xpos() = 0.0;
  ship.ypos() = 0.0;
  ship.armor() = 100;
  ship.size() = Shipdata[ShipType::OTYPE_FACTORY][ABIL_BUILD];

  ShipRepository ships(store);
  ships.save(ship);

  // Create test sectormap
  {
    SectorMap smap(planet, true);
    smap.get(5, 5).set_owner(1);
    smap.get(5, 5).set_popn(1000);
    smap.get(5, 5).set_troops(500);
    smap.get(5, 5).set_condition(SectorType::SEC_MOUNT);

    SectorRepository sectors(store);
    sectors.save_map(smap);
  }

  // Create GameObj
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Test defend command
  command_t argv = {"defend", "1", "5,5", "25"};
  GB::commands::defend(argv, g);

  // Verify planet destruct decreased
  ctx.em.clear_cache();
  const auto* saved_planet = ctx.em.peek_planet(0, 0);
  assert(saved_planet);
  assert(saved_planet->info(player_t{1}).destruct < 100);

  std::println("defend_test passed!");
  return 0;
}
