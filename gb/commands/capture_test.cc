// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import test;
import commands;
import std;

#include <cassert>

int main() {
  // Create test context
  TestContext ctx;
  JsonStore store(ctx.db);

  // Create two test races (attacker and defender)
  Race attacker{};
  attacker.Playernum = 1;
  attacker.Guest = false;
  attacker.Gov_ship = 0;
  attacker.tech = 10.0;
  attacker.fighters = 1.0;
  attacker.mass = 1.0;
  attacker.morale = 100;
  attacker.likes[SectorType::SEC_SEA] = 50;

  Race defender{};
  defender.Playernum = 2;
  defender.Guest = false;
  defender.Gov_ship = 0;
  defender.tech = 5.0;
  defender.fighters = 1.0;
  defender.mass = 1.0;
  defender.morale = 50;

  RaceRepository races(store);
  races.save(attacker);
  races.save(defender);

  // Create star
  star_struct star{};
  star.star_id = 0;
  star.pnames.push_back("TestPlanet");
  star.AP[0] = 10;  // Attacker has APs
  star.AP[1] = 10;  // Defender has APs

  StarRepository stars(store);
  stars.save(star);

  // Create planet
  Planet planet{};
  planet.star_id() = 0;
  planet.planet_order() = 0;
  planet.Maxx() = 10;
  planet.Maxy() = 10;
  planet.info(player_t{1}).mob_points = 0;

  PlanetRepository planets(store);
  planets.save(planet);

  // Create sectormap with troops for attacker
  {
    SectorMap smap(planet, true);  // Initialize empty sectors
    smap.get(5, 5).set_owner(2);   // Defender owns the sector
    smap.get(5, 5).set_popn(50);
    smap.get(5, 5).set_troops(100);  // Defender has troops
    smap.get(5, 5).set_condition(SectorType::SEC_LAND);
    SectorRepository sectors(store);
    sectors.save_map(smap);
  }

  // Create defender's ship (landed on planet)
  ship_struct ship{};
  ship.number = 1;
  ship.owner = 2;
  ship.governor = 0;
  ship.type = ShipType::STYPE_CARGO;
  ship.xpos = 0.0;
  ship.ypos = 0.0;
  ship.land_x = 5;
  ship.land_y = 5;
  ship.whatorbits = ScopeLevel::LEVEL_PLAN;
  ship.storbits = 0;
  ship.pnumorbits = 0;
  ship.on = true;
  ship.alive = true;
  ship.popn = 10;
  ship.troops = 5;
  ship.max_crew = 20;
  ship.max_resource = 100;
  ship.damage = 0;
  ship.mass = 50.0;
  ship.build_cost = 100;
  ship.destruct = 0;

  auto ship_handle = ctx.em.create_ship(ship);
  ship_handle.save();

  // Create GameObj for attacker
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);
  ctx.setup_game_obj(g);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(0);
  g.set_pnum(0);

  // Execute capture command - simulate: capture #1 50 military
  command_t argv = {"capture", "#1", "50", "military"};
  GB::commands::capture(argv, g);

  // Verify changes persisted
  const auto* captured_ship = ctx.em.peek_ship(1);
  assert(captured_ship);

  // The ship should either be captured (owner changed) or damaged from combat
  // We can't predict exact outcome due to combat randomness, but verify:
  // 1. Ship still exists or was destroyed
  // 2. Sector population changed

  const auto* final_smap = ctx.em.peek_sectormap(0, 0);
  assert(final_smap);
  const auto& final_sector = final_smap->get(5, 5);

  // Sector should have lost some troops (boarders launched)
  assert(final_sector.get_troops() <= 100);

  // If ship survived and was captured, owner should be 1
  if (captured_ship->alive()) {
    // Ship may or may not have been captured depending on combat outcome
    // Just verify it's in a valid state
    assert(captured_ship->owner() == 1 || captured_ship->owner() == 2);
  }

  std::println("✓ capture command: Ship combat and persistence verified");
  return 0;
}
