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
  race.name = "Lander";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Setup universe
  UniverseRepository universe_repo(store);
  universe_struct sdata{};
  sdata.id = 1;
  sdata.numstars = 1;
  universe_repo.save(sdata);

  // Create test star with APs
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "LandingStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);
  ss.AP[0] = 10;  // Give player 1 enough APs
  ss.governor[0] = 0;
  ss.pnames.emplace_back("LandingPlanet");
  Star star(ss);

  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create test planet
  planet_struct ps{};
  ps.star_id = 0;
  ps.planet_order = 0;
  ps.type = PlanetType::EARTH;
  ps.Maxx = 10;
  ps.Maxy = 10;
  ps.xpos = 5.0;
  ps.ypos = 5.0;
  ps.info[0].explored = true;
  ps.info[0].numsectsowned = 5;
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create sectormap for the planet
  SectorMap smap(planet, true);
  SectorRepository sector_repo(store);
  sector_repo.save_map(smap);

  // Create a ship that can land (shuttle)
  Ship shuttle{};
  shuttle.number() = 1;
  shuttle.owner() = 1;
  shuttle.governor() = 0;
  shuttle.alive() = true;
  shuttle.active() = true;
  shuttle.type() = ShipType::STYPE_SHUTTLE;  // Can land
  shuttle.build_type() = ShipType::STYPE_SHUTTLE;
  shuttle.name() = "TestShuttle";
  shuttle.damage() = 0.0;
  shuttle.armor() = 1;
  shuttle.size() = 10;
  shuttle.max_crew() = 10;
  shuttle.max_resource() = 100;
  shuttle.max_fuel() = 100;
  shuttle.max_destruct() = 10;
  shuttle.max_speed() = 10;
  shuttle.max_hanger() = 0;
  shuttle.base_mass() = 1.0;
  shuttle.mass() = 1.0;
  shuttle.fuel() = 50.0;  // Within max_fuel
  shuttle.resource() = 0;
  shuttle.destruct() = 0;
  shuttle.popn() = 2;  // Within max_crew
  shuttle.troops() = 0;
  shuttle.whatorbits() = ScopeLevel::LEVEL_PLAN;
  shuttle.storbits() = 0;
  shuttle.pnumorbits() = 0;
  shuttle.xpos() = 105.0;  // Close to planet
  shuttle.ypos() = 205.0;
  shuttle.speed() = 5;
  shuttle.docked() = false;

  ShipRepository ships_repo(store);
  ships_repo.save(shuttle);

  // Create GameObj
  GameObj g(em);
  g.player = 1;
  g.governor = 0;
  g.race = em.peek_race(1);
  g.level = ScopeLevel::LEVEL_PLAN;
  g.snum = 0;
  g.pnum = 0;
  g.shipno = 1;

  std::println("Test 1: Land ship on planet coordinates");
  {
    command_t argv = {"land", "#1", "5,5"};
    GB::commands::land(argv, g);

    // Check what the command output was
    std::string output = g.out.str();
    std::println("Command output: {}", output);

    const auto* s = em.peek_ship(1);
    assert(s != nullptr);
    // Ship should be docked after landing
    assert(s->docked());
    assert(s->land_x() == 5);
    assert(s->land_y() == 5);
    std::println("✓ Ship landed on planet coordinates");
  }

  std::println("Test 2: Cannot land docked ship");
  {
    // Ship is already docked from test 1
    const auto* s_before = em.peek_ship(1);
    bool was_docked = s_before->docked();

    command_t argv = {"land", "#1", "3,3"};
    GB::commands::land(argv, g);

    // Should still be at original location
    const auto* s_after = em.peek_ship(1);
    assert(s_after->docked() == was_docked);
    std::println("✓ Cannot re-land already docked ship");
  }

  std::println("Test 3: Create carrier and shuttle for friendly landing");
  {
    g.out.str("");  // Clear output from previous tests
    g.out.clear();

    // Reset shuttle to undocked state
    {
      auto s_handle = em.get_ship(1);
      auto& s = *s_handle;
      s.docked() = false;
      s.whatorbits() = ScopeLevel::LEVEL_PLAN;
      s.land_x() = 5;
      s.land_y() = 5;
    }

    // Create a carrier
    Ship carrier{};
    carrier.number() = 2;
    carrier.owner() = 1;
    carrier.governor() = 0;
    carrier.alive() = true;
    carrier.active() = true;
    carrier.type() = ShipType::STYPE_CARRIER;
    carrier.name() = "TestCarrier";
    carrier.damage() = 0.0;
    carrier.mass() = 1000.0;
    carrier.max_hanger() = 20;  // Capacity for 20 size units
    carrier.hanger() = 0;       // Empty hanger (current usage)
    carrier.whatorbits() = ScopeLevel::LEVEL_PLAN;
    carrier.storbits() = 0;
    carrier.pnumorbits() = 0;
    carrier.xpos() = 105.0;
    carrier.ypos() = 205.0;
    carrier.land_x() = 5;
    carrier.land_y() = 5;
    carrier.docked() = true;  // Carrier is landed

    ships_repo.save(carrier);

    // Now the shuttle (already at 5,5 landed) can land on carrier
    command_t argv = {"land", "#1", "#2"};
    GB::commands::land(argv, g);

    // Check what the command output was
    std::string output = g.out.str();
    std::println("Command output: {}", output);

    const auto* shuttle_after = em.peek_ship(1);
    assert(shuttle_after->docked());
    std::println("✓ Ship can land on friendly carrier");
  }

  std::println("All land tests passed!");
  return 0;
}
