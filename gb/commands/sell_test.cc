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

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "Trader";
  race.Guest = false;
  race.governor[0].active = true;

  RaceRepository races(store);
  races.save(race);

  // Create test star with APs
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TradeHub";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);
  ss.AP[0] = 100;  // Give player 1 enough APs (sell uses 20 per command)
  ss.pnames.push_back("TradePlanet");
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
  ps.info[0].explored = true;
  ps.info[0].numsectsowned = 5;
  ps.info[0].resource = 1000;
  ps.info[0].fuel = 500;
  ps.info[0].destruct = 200;
  ps.info[0].crystals = 50;
  Planet planet(ps);

  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create a space port ship
  Ship port{};
  port.number() = 1;
  port.owner() = 1;
  port.governor() = 0;
  port.alive() = true;
  port.active() = true;
  port.type() = ShipType::OTYPE_GOV;  // Has ABIL_PORT capability
  port.damage() = 0.0;
  port.whatorbits() = ScopeLevel::LEVEL_PLAN;
  port.storbits() = 0;
  port.pnumorbits() = 0;

  ShipRepository ships_repo(store);
  ships_repo.save(port);

  // Link ship to planet
  {
    auto planet_handle = em.get_planet(0, 0);
    auto& p = *planet_handle;
    p.ships() = 1;
  }

  // Create GameObj
  GameObj g(em);
  g.set_player(1);
  g.set_governor(0);
  g.race = em.peek_race(1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  std::println("Test 1: Sell resources");
  {
    const auto* p_before = em.peek_planet(0, 0);
    int initial_resource = p_before->info(0).resource;

    command_t argv = {"sell", "r", "100"};
    GB::commands::sell(argv, g);

    const auto* p_after = em.peek_planet(0, 0);
    assert(p_after->info(0).resource == initial_resource - 100);
    std::println("✓ Resources sold and deducted from planet");
  }

  std::println("Test 2: Sell fuel");
  {
    const auto* p_before = em.peek_planet(0, 0);
    int initial_fuel = p_before->info(0).fuel;

    command_t argv = {"sell", "f", "50"};
    GB::commands::sell(argv, g);

    const auto* p_after = em.peek_planet(0, 0);
    assert(p_after->info(0).fuel == initial_fuel - 50);
    std::println("✓ Fuel sold and deducted from planet");
  }

  std::println("Test 3: Sell destruct");
  {
    const auto* p_before = em.peek_planet(0, 0);
    int initial_destruct = p_before->info(0).destruct;

    command_t argv = {"sell", "d", "25"};
    GB::commands::sell(argv, g);

    const auto* p_after = em.peek_planet(0, 0);
    assert(p_after->info(0).destruct == initial_destruct - 25);
    std::println("✓ Destruct sold and deducted from planet");
  }

  std::println("Test 4: Sell crystals");
  {
    const auto* p_before = em.peek_planet(0, 0);
    int initial_crystals = p_before->info(0).crystals;

    command_t argv = {"sell", "x", "10"};
    GB::commands::sell(argv, g);

    const auto* p_after = em.peek_planet(0, 0);
    assert(p_after->info(0).crystals == initial_crystals - 10);
    std::println("✓ Crystals sold and deducted from planet");
  }

  std::println("Test 5: Guest race cannot sell");
  {
    auto race_handle = em.get_race(1);
    auto& r = *race_handle;
    r.Guest = true;
  }
  {
    GameObj g2(em);
    g2.set_player(1);
    g2.set_governor(0);
    g2.race = em.peek_race(1);
    g2.set_level(ScopeLevel::LEVEL_PLAN);
    g2.set_snum(0);
    g2.set_pnum(0);

    const auto* p_before = em.peek_planet(0, 0);
    int resource_before = p_before->info(0).resource;

    command_t argv = {"sell", "r", "50"};
    GB::commands::sell(argv, g2);

    // Should not have changed
    const auto* p_after = em.peek_planet(0, 0);
    assert(p_after->info(0).resource == resource_before);
    std::println("✓ Guest race blocked from selling");
  }

  std::println("All sell tests passed!");
  return 0;
}
