// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import commands;
import std;

#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "Testers";
  race.Guest = false;
  race.governor[0].money = 10000;
  race.fighters = 100;

  RaceRepository races(store);
  races.save(race);

  // Create test star
  star_struct star{};
  star.star_id = 0;
  star.name = "Test Star";
  star.AP[0] = 100;

  StarRepository stars(store);
  stars.save(star);

  // Create test planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.info(0).destruct = 1000;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create test sectormap
  {
    SectorMap smap(planet, true);
    smap.get(5, 5).set_owner(1);
    smap.get(5, 5).set_popn(1000);
    smap.get(5, 5).set_troops(0);
    smap.get(5, 5).set_mobilization(1);
    smap.get(5, 5).set_condition(SectorType::SEC_MOUNT);

    SectorRepository sectors(store);
    sectors.save_map(smap);
  }

  // Create GameObj
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_PLAN;
  g.snum = 0;
  g.pnum = 0;

  // Test arm command
  command_t argv = {"arm", "5,5", "100"};
  GB::commands::arm(argv, g);

  // Verify changes persisted
  em.clear_cache();
  const auto* saved_smap = em.peek_sectormap(0, 0);
  assert(saved_smap);
  const auto& saved_sect = saved_smap->get(5, 5);

  assert(saved_sect.get_troops() == 100);
  assert(saved_sect.get_popn() == 900);

  const auto* saved_planet = em.peek_planet(0, 0);
  assert(saved_planet);
  assert(saved_planet->troops() == 100);

  const auto* saved_race = em.peek_race(1);
  assert(saved_race);
  assert(saved_race->governor[0].money == 0);

  // Test disarm
  command_t argv2 = {"disarm", "5,5", "50"};
  GB::commands::arm(argv2, g);

  em.clear_cache();
  saved_smap = em.peek_sectormap(0, 0);
  const auto& saved_sect2 = saved_smap->get(5, 5);
  assert(saved_sect2.get_troops() == 50);
  assert(saved_sect2.get_popn() == 950);

  std::println("arm_test passed!");
  return 0;
}
