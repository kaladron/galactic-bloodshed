// SPDX-License-Identifier: Apache-2.0

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

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "LoadTester";
  race.Guest = false;
  race.governor[0].active = true;
  race.mass = 1.0;
  race.absorb = false;
  race.Metamorph = false;

  RaceRepository races(store);
  races.save(race);

  // Create test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "LoadStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.pnames.emplace_back("LoadPlanet");
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create test planet with resources
  planet_struct ps{};
  ps.star_id = 0;
  ps.planet_order = 0;
  ps.type = PlanetType::EARTH;
  ps.Maxx = 10;
  ps.Maxy = 10;
  ps.info[0].fuel = 1000;
  ps.info[0].resource = 500;
  ps.info[0].destruct = 200;
  ps.info[0].crystals = 50;
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create a landed ship to load cargo onto
  Ship ship{};
  ship.number() = 1;
  ship.owner() = 1;
  ship.governor() = 0;
  ship.alive() = true;
  ship.active() = true;
  ship.type() = ShipType::STYPE_CARGO;
  ship.name() = "CargoHauler";
  ship.whatorbits() = ScopeLevel::LEVEL_PLAN;
  ship.storbits() = 0;
  ship.pnumorbits() = 0;
  ship.whatdest() = ScopeLevel::LEVEL_PLAN;  // Important: must be PLAN for planet loading
  ship.deststar() = 0;
  ship.destpnum() = 0;
  ship.land_x() = 5;
  ship.land_y() = 5;
  ship.docked() = 1;  // CRITICAL: Ship must be docked to load/unload
  ship.fuel() = 100.0;
  ship.max_fuel() = 500.0;
  ship.resource() = 0;
  ship.max_resource() = 1000;
  ship.destruct() = 0;
  ship.max_destruct() = 300;
  ship.crystals() = 0;
  ship.mass() = 100.0;

  ShipRepository ships_repo(store);
  ships_repo.save(ship);

  // Create GameObj
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_PLAN;
  g.snum = 0;
  g.pnum = 0;

  std::println("Test 1: Load fuel from planet to ship");
  {
    const auto* s_before = em.peek_ship(1);
    const auto* p_before = em.peek_planet(0, 0);
    double initial_ship_fuel = s_before->fuel();
    int initial_planet_fuel = p_before->info(0).fuel;

    command_t argv = {"load", "#1", "f", "100"};
    GB::commands::load(argv, g);

    const auto* s_after = em.peek_ship(1);
    const auto* p_after = em.peek_planet(0, 0);
    assert(s_after->fuel() == initial_ship_fuel + 100);
    assert(p_after->info(0).fuel == initial_planet_fuel - 100);
    std::println("✓ Fuel loaded from planet to ship");
  }

  std::println("Test 2: Load resources from planet to ship");
  {
    const auto* s_before = em.peek_ship(1);
    const auto* p_before = em.peek_planet(0, 0);
    int initial_ship_resource = s_before->resource();
    int initial_planet_resource = p_before->info(0).resource;

    command_t argv = {"load", "#1", "r", "200"};
    GB::commands::load(argv, g);

    const auto* s_after = em.peek_ship(1);
    const auto* p_after = em.peek_planet(0, 0);
    assert(s_after->resource() == initial_ship_resource + 200);
    assert(p_after->info(0).resource == initial_planet_resource - 200);
    std::println("✓ Resources loaded from planet to ship");
  }

  std::println("Test 3: Load destruct from planet to ship");
  {
    const auto* s_before = em.peek_ship(1);
    const auto* p_before = em.peek_planet(0, 0);
    int initial_ship_destruct = s_before->destruct();
    int initial_planet_destruct = p_before->info(0).destruct;

    command_t argv = {"load", "#1", "d", "50"};
    GB::commands::load(argv, g);

    const auto* s_after = em.peek_ship(1);
    const auto* p_after = em.peek_planet(0, 0);
    assert(s_after->destruct() == initial_ship_destruct + 50);
    assert(p_after->info(0).destruct == initial_planet_destruct - 50);
    std::println("✓ Destruct loaded from planet to ship");
  }

  std::println("Test 4: Load crystals from planet to ship");
  {
    const auto* s_before = em.peek_ship(1);
    const auto* p_before = em.peek_planet(0, 0);
    int initial_ship_crystals = s_before->crystals();
    int initial_planet_crystals = p_before->info(0).crystals;

    command_t argv = {"load", "#1", "x", "10"};
    GB::commands::load(argv, g);

    const auto* s_after = em.peek_ship(1);
    const auto* p_after = em.peek_planet(0, 0);
    assert(s_after->crystals() == initial_ship_crystals + 10);
    assert(p_after->info(0).crystals == initial_planet_crystals - 10);
    std::println("✓ Crystals loaded from planet to ship");
  }

  std::println("\n✅ All load command tests passed!");
  std::println("The load command correctly transfers cargo and persists changes via EntityManager.");
  return 0;
}
