// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import commands;
import std.compat;

#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  // Create test race via repository
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  RaceRepository races(store);
  races.save(race);

  // Create test star via repository
  StarRepository stars(store);
  star_struct star1_data{};
  star1_data.star_id = 1;
  star1_data.name = "TestStar";
  star1_data.pnames.push_back("TestPlanet");
  Star star1(star1_data);
  stars.save(star1);

  // Create destination star for route
  star_struct star2_data{};
  star2_data.star_id = 2;
  star2_data.name = "DestStar";
  star2_data.pnames.push_back("DestPlanet");
  Star star2(star2_data);
  stars.save(star2);

  // Create test planet via repository
  PlanetRepository planets(store);
  Planet planet{};
  planet.star_id() = 1;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.info(0).route[0].set = 0;
  planet.info(0).route[0].dest_star = 0;
  planet.info(0).route[0].dest_planet = 0;
  planet.info(0).route[0].x = 0;
  planet.info(0).route[0].y = 0;
  planet.info(0).route[0].load = 0;
  planet.info(0).route[0].unload = 0;
  planets.save(planet);

  // Test: Set route destination
  {
    auto planet_handle = em.get_planet(1, 0);
    auto& p = *planet_handle;
    p.info(0).route[0].set = 1;
    p.info(0).route[0].dest_star = 2;
    p.info(0).route[0].dest_planet = 0;
    p.info(0).route[0].x = 5;
    p.info(0).route[0].y = 5;
    p.info(0).route[0].load = M_FUEL | M_RESOURCES;
    p.info(0).route[0].unload = M_DESTRUCT;
  }

  // Verify: Route was saved
  {
    const auto* saved = em.peek_planet(1, 0);
    assert(saved);
    assert(saved->info(0).route[0].set == 1);
    assert(saved->info(0).route[0].dest_star == 2);
    assert(saved->info(0).route[0].dest_planet == 0);
    assert(saved->info(0).route[0].x == 5);
    assert(saved->info(0).route[0].y == 5);
    assert(saved->info(0).route[0].load == (M_FUEL | M_RESOURCES));
    assert(saved->info(0).route[0].unload == M_DESTRUCT);
    std::println("✓ Route destination saved correctly");
  }

  // Test: Deactivate route
  {
    auto planet_handle = em.get_planet(1, 0);
    auto& p = *planet_handle;
    p.info(0).route[0].set = 0;
  }

  // Verify: Route deactivated
  {
    const auto* saved = em.peek_planet(1, 0);
    assert(saved);
    assert(saved->info(0).route[0].set == 0);
    std::println("✓ Route deactivation saved correctly");
  }

  // Test: Multiple routes
  {
    auto planet_handle = em.get_planet(1, 0);
    auto& p = *planet_handle;
    for (int i = 0; i < MAX_ROUTES; i++) {
      p.info(0).route[i].set = 1;
      p.info(0).route[i].dest_star = 2;
      p.info(0).route[i].dest_planet = 0;
      p.info(0).route[i].load = M_FUEL;
    }
  }

  // Verify: All routes saved
  {
    const auto* saved = em.peek_planet(1, 0);
    assert(saved);
    for (int i = 0; i < MAX_ROUTES; i++) {
      assert(saved->info(0).route[i].set == 1);
      assert(saved->info(0).route[i].load == M_FUEL);
    }
    std::println("✓ Multiple routes saved correctly");
  }

  std::println("All route tests passed!");
  return 0;
}
