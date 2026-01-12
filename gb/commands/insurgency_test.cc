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

  // Create test race (instigator)
  Race race{};
  race.Playernum = 1;
  race.name = "Rebels";
  race.Guest = false;
  race.governor[0].money = 10000;
  race.morale = 100;
  race.fighters = 10;

  RaceRepository races(store);
  races.save(race);

  // Create target race
  Race target{};
  target.Playernum = 2;
  target.name = "Oppressors";
  target.Guest = false;
  target.morale = 50;
  target.fighters = 5;
  races.save(target);

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
  planet.info(player_t{1}).popn = 100;
  planet.info(player_t{1}).troops = 50;
  planet.info(player_t{2}).popn = 1000;
  planet.info(player_t{2}).troops = 100;
  planet.info(player_t{2}).numsectsowned = 5;
  planet.info(player_t{2}).tax = 10;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create test sectormap
  {
    SectorMap smap(planet, true);
    for (int i = 0; i < 5; i++) {
      smap.get(i, 0).set_owner(2);
      smap.get(i, 0).set_popn(200);
      smap.get(i, 0).set_condition(SectorType::SEC_MOUNT);
    }

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

  // Test insurgency command
  command_t argv = {"insurgency", "2", "5000"};
  GB::commands::insurgency(argv, g);

  // Verify race money decreased
  ctx.em.clear_cache();
  const auto* saved_race = ctx.em.peek_race(1);
  assert(saved_race);
  assert(saved_race->governor[0].money == 5000);

  std::println("insurgency_test passed!");
  return 0;
}
