// SPDX-License-Identifier: Apache-2.0

#include <glaze/glaze.hpp>

import gblib;
import std.compat;

#include <cassert>

int main() {
  // Test plinfo route serialization
  plinfo test_plinfo{};
  test_plinfo.fuel = 100;
  test_plinfo.destruct = 50;
  test_plinfo.resource = 1000;
  test_plinfo.popn = 5000;
  test_plinfo.troops = 200;
  test_plinfo.crystals = 10;
  test_plinfo.prod_res = 25;
  test_plinfo.prod_fuel = 15;
  test_plinfo.prod_dest = 5;
  test_plinfo.prod_crystals = 2;
  test_plinfo.prod_money = 500;
  test_plinfo.prod_tech = 10.5;
  test_plinfo.tech_invest = 1000;
  test_plinfo.numsectsowned = 50;
  test_plinfo.comread = 75;
  test_plinfo.mob_set = 80;
  test_plinfo.tox_thresh = 20;
  test_plinfo.explored = 1;
  test_plinfo.autorep = 1;
  test_plinfo.tax = 10;
  test_plinfo.newtax = 12;
  test_plinfo.guns = 5;
  test_plinfo.mob_points = 10000;
  test_plinfo.est_production = 123.45;
  
  // Set up a route
  test_plinfo.route[0].set = 1;
  test_plinfo.route[0].dest_star = 5;
  test_plinfo.route[0].dest_planet = 3;
  test_plinfo.route[0].load = 0x0F;
  test_plinfo.route[0].unload = 0xF0;
  test_plinfo.route[0].x = 10;
  test_plinfo.route[0].y = 20;

  // Serialize plinfo to JSON
  auto json_result = glz::write_json(test_plinfo);
  assert(json_result.has_value());
  std::println("plinfo JSON: {}", json_result.value());

  // Deserialize back
  plinfo recovered_plinfo{};
  auto parse_result = glz::read_json(recovered_plinfo, json_result.value());
  assert(!parse_result);

  // Verify data
  assert(recovered_plinfo.fuel == 100);
  assert(recovered_plinfo.destruct == 50);
  assert(recovered_plinfo.resource == 1000);
  assert(recovered_plinfo.popn == 5000);
  assert(recovered_plinfo.troops == 200);
  assert(recovered_plinfo.route[0].set == 1);
  assert(recovered_plinfo.route[0].dest_star == 5);
  assert(recovered_plinfo.route[0].dest_planet == 3);
  assert(recovered_plinfo.route[0].x == 10);
  assert(recovered_plinfo.route[0].y == 20);

  std::println("✓ plinfo serialization test passed");

  // Test Planet serialization
  Planet test_planet(PlanetType::EARTH);
  test_planet.planet_id = 42;
  test_planet.xpos = 100.5;
  test_planet.ypos = 200.7;
  test_planet.ships = 10;
  test_planet.Maxx = 20;
  test_planet.Maxy = 20;
  test_planet.popn = 100000;
  test_planet.troops = 5000;
  test_planet.maxpopn = 150000;
  test_planet.total_resources = 50000;
  test_planet.slaved_to = 0;
  test_planet.expltimer = 5;
  test_planet.explored = 1;
  
  // Set conditions
  test_planet.conditions[TEMP] = 50;
  test_planet.conditions[OXYGEN] = 20;
  test_planet.conditions[CO2] = 5;
  
  // Set some player info
  test_planet.info[1].fuel = 500;
  test_planet.info[1].popn = 10000;
  test_planet.info[1].numsectsowned = 100;
  test_planet.info[1].route[0].set = 1;
  test_planet.info[1].route[0].dest_star = 3;

  // Serialize Planet to JSON - use const reference
  auto planet_json_result = glz::write_json<const Planet&>(test_planet);
  assert(planet_json_result.has_value());
  std::println("\nPlanet JSON length: {} bytes", planet_json_result.value().size());
  
  // Show a snippet of the JSON (first 200 chars)
  std::string snippet = planet_json_result.value().substr(0, std::min(200ul, planet_json_result.value().size()));
  std::println("Planet JSON snippet: {}...", snippet);

  // Deserialize back
  Planet recovered_planet(PlanetType::EARTH);
  auto planet_parse_result = glz::read_json(recovered_planet, planet_json_result.value());
  assert(!planet_parse_result);

  // Verify data
  assert(recovered_planet.planet_id == 42);
  assert(recovered_planet.xpos == 100.5);
  assert(recovered_planet.ypos == 200.7);
  assert(recovered_planet.ships == 10);
  assert(recovered_planet.Maxx == 20);
  assert(recovered_planet.Maxy == 20);
  assert(recovered_planet.popn == 100000);
  assert(recovered_planet.troops == 5000);
  assert(recovered_planet.type == PlanetType::EARTH);
  assert(recovered_planet.conditions[TEMP] == 50);
  assert(recovered_planet.conditions[OXYGEN] == 20);
  assert(recovered_planet.info[1].fuel == 500);
  assert(recovered_planet.info[1].popn == 10000);
  assert(recovered_planet.info[1].route[0].set == 1);
  assert(recovered_planet.info[1].route[0].dest_star == 3);

  std::println("✓ Planet serialization test passed");
  std::println("\nAll tests passed!");
  return 0;
}
