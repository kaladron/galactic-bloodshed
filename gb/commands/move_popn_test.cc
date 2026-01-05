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
  race.fighters = 10;

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

  PlanetRepository planets(store);
  planets.save(planet);

  // Create test sectormap
  {
    SectorMap smap(planet, true);

    smap.get(5, 5).set_owner(1);
    smap.get(5, 5).set_popn(1000);
    smap.get(5, 5).set_troops(0);
    smap.get(5, 5).set_condition(SectorType::SEC_MOUNT);

    smap.get(5, 6).set_owner(1);
    smap.get(5, 6).set_popn(0);
    smap.get(5, 6).set_troops(0);
    smap.get(5, 6).set_condition(SectorType::SEC_MOUNT);

    SectorRepository sectors(store);
    sectors.save_map(smap);
  }

  // Create GameObj
  auto* registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Test move command - 'k' moves south (y+1)
  command_t argv = {"move", "5,5", "k", "500"};
  GB::commands::move_popn(argv, g);

  // Verify population moved
  ctx.em.clear_cache();
  const auto* saved_smap = ctx.em.peek_sectormap(0, 0);
  assert(saved_smap);

  const auto& source_sect = saved_smap->get(5, 5);
  assert(source_sect.get_popn() == 500);

  const auto& dest_sect = saved_smap->get(5, 6);
  assert(dest_sect.get_popn() == 500);

  std::println("move_popn_test passed!");
  return 0;
}
