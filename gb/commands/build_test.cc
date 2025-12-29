// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import commands;
import std;

#include <cassert>

int main() {
  // Initialize database
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);

  // Create a test race
  Race race{};
  race.Playernum = 1;
  race.governor[0].active = true;
  race.name = "TestRace";
  race.Guest = false;
  race.God = false;
  race.tech = 500.0;  // High tech to build any ship
  race.pods = false;

  JsonStore store(db);
  RaceRepository races(store);
  races.save(race);

  // Create a test star
  star_struct star_data{};
  star_data.star_id = 0;
  star_data.governor[0] = 0;
  star_data.name = "TestStar";
  star_data.xpos = 100.0;
  star_data.ypos = 100.0;
  Star star{star_data};
  StarRepository stars_repo(store);
  stars_repo.save(star);
  const starnum_t star_id = star_data.star_id;

  // Create a test planet with resources
  Planet planet{};
  planet.star_id() = star_id;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.xpos() = 0.0;
  planet.ypos() = 0.0;
  planet.info(0).resource = 10000;  // Plenty of resources
  planet.info(0).fuel = 1000;

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create a sectormap with a sector with population for building
  {
    SectorMap smap(planet, true);  // Initialize empty sectors
    smap.get(5, 5).set_owner(1);
    smap.get(5, 5).set_popn(100);
    smap.get(5, 5).set_condition(SectorType::SEC_LAND);
    SectorRepository sectors_repo(store);
    sectors_repo.save_map(smap);
  }

  // Create GameObj for testing
  GameObj g(em);
  g.set_player(1);
  g.set_governor(0);
  g.race = em.peek_race(1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(star_id);
  g.set_pnum(0);

  // Test: Build a probe on planet
  {
    command_t argv = {"build", ":", "5,5", "1"};  // ":" = Probe
    GB::commands::build(argv, g);

    // The build command uses notify() which sends to connected clients,
    // not g.out. In tests, we verify success by checking the database
    // instead of output messages.

    // Verify planet resources were deducted
    em.clear_cache();
    const auto* planet_verify = em.peek_planet(star_id, 0);
    assert(planet_verify);
    assert(planet_verify->info(0).resource <
           10000);  // Resources should be deducted

    // Verify ship was created (it should be ship #1)
    const auto* ship = em.peek_ship(1);
    assert(ship);
    assert(ship->type() == ShipType::OTYPE_PROBE);
    assert(ship->owner() == 1);
    assert(ship->whatorbits() == ScopeLevel::LEVEL_PLAN);
    assert(ship->storbits() == star_id);
    assert(ship->pnumorbits() == 0);
    assert(ship->land_x() == 5);
    assert(ship->land_y() == 5);

    std::println("Build command test passed: Probe built on planet");
  }

  // Test: Build with insufficient resources
  {
    // Drain resources completely
    auto planet_handle2 = em.get_planet(star_id, 0);
    auto& planet2 = *planet_handle2;
    planet2.info(0).resource = 0;  // No resources
  }

  {
    command_t argv = {"build", ":", "5,5",
                      "1"};  // Try to build probe with no resources
    GB::commands::build(argv, g);

    // The build command should fail due to insufficient resources.
    // The error is written to g.out
    std::string result = g.out.str();
    std::println("Build error output: {}", result);
    assert(result.find("You need") != std::string::npos);
    g.out.str("");

    std::println("Build command test passed: Insufficient resources");
  }

  std::println("\nAll build command tests passed!");
  return 0;
}
