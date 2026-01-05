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
  race.name = "Enslavers";
  race.Guest = false;

  RaceRepository races(store);
  races.save(race);

  // Create enemy race
  Race enemy{};
  enemy.Playernum = 2;
  enemy.name = "Victims";
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
  planet.info(0).numsectsowned = 5;
  planet.info(1).popn = 1000;
  planet.info(1).numsectsowned = 5;
  planet.info(0).destruct = 1000;
  planet.info(1).destruct = 100;
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

  ShipRepository ships(store);
  ships.save(oap);

  // Create GameObj
  auto* registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.set_snum(0);
  g.set_pnum(0);

  // Test enslave command
  command_t argv = {"enslave", "1"};
  GB::commands::enslave(argv, g);

  // Verify planet was enslaved
  ctx.em.clear_cache();
  const auto* saved_planet = ctx.em.peek_planet(0, 0);
  assert(saved_planet);
  assert(saved_planet->slaved_to() == 1);

  std::println("enslave_test passed!");
  return 0;
}
