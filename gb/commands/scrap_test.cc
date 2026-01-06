// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std.compat;

#include <cassert>

int main() {
  // Create test context
  TestContext ctx;

  // Create test race
  Race race{};
  race.Playernum = 1;
  race.name = "TestRace";
  race.governor[0].active = true;
  race.mass = 1.0;
  race.fighters = 1.0;
  race.tech = 100.0;  // High tech for testing
  race.morale = 100;

  // Save race via repository
  JsonStore store(ctx.db);
  RaceRepository races(store);
  races.save(race);

  // Create a test star
  star_struct ss{};
  ss.star_id = 0;
  ss.name = "TestStar";
  ss.xpos = 100.0;
  ss.ypos = 200.0;
  ss.explored = (1ULL << 1);  // Player 1 has explored this star
  ss.AP[0] = 10;              // Player 1 has APs (0-indexed in AP array)
  Star star(ss);

  // Save star via repository
  StarRepository stars_repo(store);
  stars_repo.save(star);

  // Create a planet for the star
  Planet planet{PlanetType::EARTH};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.popn() = 1000;
  planet.info(0).numsectsowned = 1;
  planet.info(0).popn = 1000;
  planet.info(0).resource = 500;

  // Save planet via repository
  PlanetRepository planets_repo(store);
  planets_repo.save(planet);

  // Create a sector map for the planet
  SectorMap smap(planet, true);
  auto& sector = smap.get(5, 5);
  sector.set_owner(1);
  sector.set_popn(100);
  sector.set_resource(50);
  sector.set_eff(100);

  // Save sector map using Repository (DAL layer)
  SectorRepository sector_repo(store);
  sector_repo.save_map(smap);

  // Create first ship (carrier to receive scrapped resources)
  Ship ship1{};
  ship1.number() = 1;
  ship1.owner() = 1;
  ship1.governor() = 0;
  ship1.alive() = true;
  ship1.active() = true;
  ship1.type() = ShipType::STYPE_CARRIER;
  ship1.name() = "Carrier";
  ship1.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship1.storbits() = 0;
  ship1.xpos() = 100.0;
  ship1.ypos() = 200.0;
  ship1.fuel() = 100.0;
  ship1.max_fuel() = 500.0;
  ship1.resource() = 100;
  ship1.max_resource() = 1000;
  ship1.popn() = 10;
  ship1.max_crew() = 100;
  ship1.destruct() = 0;
  ship1.max_destruct() = 100;
  ship1.mass() = 100.0;
  ship1.docked() = 1;
  ship1.whatdest() = ScopeLevel::LEVEL_SHIP;
  ship1.destshipno() = 2;

  // Create second ship (to be scrapped, docked with first)
  Ship ship2{};
  ship2.number() = 2;
  ship2.owner() = 1;
  ship2.governor() = 0;
  ship2.alive() = true;
  ship2.active() = true;
  ship2.type() = ShipType::STYPE_FIGHTER;
  ship2.build_type() = ShipType::STYPE_FIGHTER;
  ship2.name() = "ToScrap";
  ship2.whatorbits() = ScopeLevel::LEVEL_STAR;
  ship2.storbits() = 0;
  ship2.xpos() = 100.0;
  ship2.ypos() = 200.0;
  ship2.fuel() = 50.0;
  ship2.max_fuel() = 100.0;
  ship2.resource() = 20;
  ship2.max_resource() = 50;
  ship2.popn() = 5;  // Has crew to allow scrapping
  ship2.max_crew() = 10;
  ship2.destruct() = 10;
  ship2.max_destruct() = 20;
  ship2.mass() = 10.0;
  ship2.build_cost() = 100;  // Set build cost for scrap value calculation
  ship2.docked() = 1;
  ship2.whatdest() = ScopeLevel::LEVEL_SHIP;
  ship2.destshipno() = 1;

  // Save ships via repository
  ShipRepository ships_repo(store);
  ships_repo.save(ship1);
  ships_repo.save(ship2);

  // Create GameObj for command execution
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_STAR);
  g.set_snum(0);

  std::println("Test 1: Scrap a docked ship and verify resources transfer");
  {
    // Hold a race handle to keep the race in cache during the test
    // This prevents kill_ship() from evicting the race when its internal handle
    // is released
    auto race_handle = ctx.em.get_race(1);
    g.race = &race_handle.read();  // Set g.race to point to the cached race

    const auto* carrier_before = ctx.em.peek_ship(1);
    assert(carrier_before != nullptr);
    int initial_resource = carrier_before->resource();
    double initial_fuel = carrier_before->fuel();
    std::println("    Carrier before: resource={}, fuel={:.0f}",
                 initial_resource, initial_fuel);

    const auto* scrap_ship = ctx.em.peek_ship(2);
    assert(scrap_ship != nullptr);
    std::println("    Ship to scrap: resource={}, fuel={:.0f}, build_cost={}",
                 scrap_ship->resource(), scrap_ship->fuel(),
                 scrap_ship->build_cost());

    // scrap #2
    command_t argv = {"scrap", "#2"};
    GB::commands::scrap(argv, g);

    // Clear cache to force reload from database
    ctx.em.clear_cache();

    // Verify ship 2 is dead
    const auto* scrapped = ctx.em.peek_ship(2);
    assert(scrapped != nullptr);
    assert(scrapped->alive() == 0);
    std::println("    ✓ Ship 2 is now dead (alive={})", scrapped->alive());

    // Verify carrier received resources
    const auto* carrier_after = ctx.em.peek_ship(1);
    assert(carrier_after != nullptr);
    std::println("    Carrier after: resource={}, fuel={:.0f}",
                 carrier_after->resource(), carrier_after->fuel());

    // Resources should have increased (scrapval = build_cost/2 + resource)
    assert(carrier_after->resource() > initial_resource);
    std::println("    ✓ Carrier resource increased from {} to {}",
                 initial_resource, carrier_after->resource());

    // Carrier should be undocked after scrap
    assert(carrier_after->docked() == 0);
    std::println("    ✓ Carrier is now undocked");
  }

  std::println("\n✅ All scrap tests passed!");
  return 0;
}
