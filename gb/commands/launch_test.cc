// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import commands;
import std;

#include <cassert>

int main() {
  // Initialize in-memory database
  Database db(":memory:");
  initialize_schema(db);

  EntityManager em(db);
  JsonStore store(db);

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.Guest = false;
  race.governor[0].money = 10000;
  race.governor[0].toggle.highlight = true;
  RaceRepository races(store);
  races.save(race);

  // Create test star
  star_struct ss{};
  ss.star_id = 0;
  ss.pnames.emplace_back(
      "TestPlanet");  // numplanets is derived from pnames.size()
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  Star star(ss);
  star.set_name("TestStar");
  StarRepository stars(store);
  stars.save(star);

  // Create test planet
  planet_struct ps{};
  ps.star_id = 0;
  ps.planet_order = 0;
  ps.Maxx = 10;
  ps.Maxy = 10;
  ps.info[0].numsectsowned = 5;
  ps.xpos = 10.0;
  ps.ypos = 20.0;
  ps.explored = 0;
  Planet planet(ps);
  PlanetRepository planets(store);
  planets.save(planet);

  // Create test ship that's landed on the planet
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;   // CRITICAL: Ship must be alive
  ship.active() = true;  // CRITICAL: Ship must be active
  ship.type() = ShipType::STYPE_SHUTTLE;
  ship.max_speed() = 5;  // CRITICAL: Ship needs speed_rating to be launchable
  ship.xpos() = 110.0;
  ship.ypos() = 220.0;
  ship.land_x() = 5;
  ship.land_y() = 5;
  ship.fuel() = 1000.0;
  ship.mass() = 100.0;
  ship.docked() = 1;
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.storbits() = 0;
  ship.pnumorbits() = 0;
  ship.whatdest() = ScopeLevel::LEVEL_PLAN;
  ship.deststar() = 0;
  ship.destpnum() = 0;
  ShipRepository ships(store);
  ships.save(ship);

  // Create GameObj
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_PLAN;
  g.snum = 0;
  g.pnum = 0;

  // Initialize Sdata for AP tracking - star AP is managed by EntityManager
  auto star_handle = em.get_star(0);
  auto& star_data = *star_handle;
  star_data.AP(0) = 100;

  // Test launching the ship
  command_t cmd{"launch", "#1"};
  GB::commands::launch(cmd, g);

  // Print output for debugging
  std::println("Command output: {}", g.out.str());

  // Verify ship is no longer docked and has fuel consumed
  const auto* launched_ship = em.peek_ship(1);
  assert(launched_ship);
  std::println("Ship docked status: {}", launched_ship->docked());
  std::println("Ship whatdest: {}",
               static_cast<int>(launched_ship->whatdest()));
  assert(launched_ship->docked() == 0);
  assert(launched_ship->whatdest() == ScopeLevel::LEVEL_UNIV);
  assert(launched_ship->fuel() < 1000.0);  // Fuel consumed

  // Verify planet is now explored
  const auto* explored_planet = em.peek_planet(0, 0);
  assert(explored_planet);
  assert(explored_planet->explored() == 1);

  std::println("✓ Ship launch persists to database");
  std::println("✓ Ship fuel consumption calculated");
  std::println("✓ Planet exploration updated");
  std::println("All launch tests passed!");

  return 0;
}
