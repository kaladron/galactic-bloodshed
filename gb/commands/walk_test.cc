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
  std::fill(std::begin(race.likes), std::end(race.likes), true);

  RaceRepository races(store);
  races.save(race);

  // Create test star
  star_struct star{};
  star.star_id = 0;
  star.name = "Test Star";
  star.pnames.emplace_back("Test Planet");
  star.AP[0] = 100;

  StarRepository stars(store);
  stars.save(star);

  // Create test planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.ships() = 1;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create AFV ship
  Ship afv{};
  afv.number() = 1;
  afv.owner() = 1;
  afv.governor() = 0;
  afv.alive() = true;
  afv.active() = true;  // Ship must be active to execute commands
  afv.type() = ShipType::OTYPE_AFV;
  afv.whatorbits() = ScopeLevel::LEVEL_PLAN;
  afv.whatdest() = ScopeLevel::LEVEL_PLAN;
  afv.docked() = true;
  afv.storbits() = 0;
  afv.pnumorbits() = 0;
  afv.land_x() = 5;
  afv.land_y() = 5;
  afv.popn() = 10;
  afv.fuel() = 100.0;
  afv.max_fuel() = 200.0;

  ShipRepository ships(store);
  ships.save(afv);

  // Create test sectormap
  {
    SectorMap smap(planet, true);
    smap.get(5, 5).set_owner(1);
    smap.get(5, 5).set_condition(SectorType::SEC_MOUNT);
    smap.get(5, 6).set_owner(1);
    smap.get(5, 6).set_condition(SectorType::SEC_MOUNT);

    SectorRepository sectors(store);
    sectors.save_map(smap);
  }

  // Create GameObj
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_UNIV;
  g.snum = 0;
  g.pnum = 0;

  // Test walk command - move south (k or '2')
  command_t argv = {"walk", "1", "k"};
  GB::commands::walk(argv, g);

  // Verify AFV moved
  em.clear_cache();
  const auto* saved_ship = em.peek_ship(1);
  assert(saved_ship);
  assert(saved_ship->land_x() == 5);
  assert(saved_ship->land_y() == 6);
  assert(saved_ship->fuel() < 100.0);

  std::println("walk_test passed!");
  return 0;
}
