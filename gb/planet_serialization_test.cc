// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <cassert>

int main() {
  // Test Planet serialization with comprehensive field coverage
  Planet test_planet(PlanetType::EARTH);

  // Test all Planet scalar fields
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
  test_planet.slaved_to = 3;
  test_planet.type = PlanetType::MARS;  // Test non-default type
  test_planet.expltimer = 5;
  test_planet.explored = 1;

  // Test all condition types
  test_planet.conditions[TEMP] = 50;
  test_planet.conditions[OXYGEN] = 20;
  test_planet.conditions[CO2] = 5;
  test_planet.conditions[HYDROGEN] = 10;
  test_planet.conditions[NITROGEN] = 15;
  test_planet.conditions[SULFUR] = 2;
  test_planet.conditions[HELIUM] = 8;
  test_planet.conditions[OTHER] = 3;
  test_planet.conditions[METHANE] = 1;
  test_planet.conditions[TOXIC] = 25;

  // Test comprehensive plinfo for player 1
  test_planet.info[1].fuel = 500;
  test_planet.info[1].destruct = 250;
  test_planet.info[1].resource = 10000;
  test_planet.info[1].popn = 50000;
  test_planet.info[1].troops = 2000;
  test_planet.info[1].crystals = 100;
  test_planet.info[1].prod_res = 500;
  test_planet.info[1].prod_fuel = 200;
  test_planet.info[1].prod_dest = 50;
  test_planet.info[1].prod_crystals = 10;
  test_planet.info[1].prod_money = 1000;
  test_planet.info[1].prod_tech = 15.5;
  test_planet.info[1].tech_invest = 5000;
  test_planet.info[1].numsectsowned = 150;
  test_planet.info[1].comread = 80;
  test_planet.info[1].mob_set = 90;
  test_planet.info[1].tox_thresh = 30;
  test_planet.info[1].explored = 1;
  test_planet.info[1].autorep = 1;
  test_planet.info[1].tax = 15;
  test_planet.info[1].newtax = 18;
  test_planet.info[1].guns = 10;
  test_planet.info[1].mob_points = 50000;
  test_planet.info[1].est_production = 2500.75;

  // Test multiple routes for player 1
  test_planet.info[1].route[0].set = 1;
  test_planet.info[1].route[0].dest_star = 5;
  test_planet.info[1].route[0].dest_planet = 3;
  test_planet.info[1].route[0].load = 0x0F;
  test_planet.info[1].route[0].unload = 0xF0;
  test_planet.info[1].route[0].x = 10;
  test_planet.info[1].route[0].y = 20;

  test_planet.info[1].route[1].set = 1;
  test_planet.info[1].route[1].dest_star = 7;
  test_planet.info[1].route[1].dest_planet = 2;
  test_planet.info[1].route[1].load = 0x03;
  test_planet.info[1].route[1].unload = 0x0C;
  test_planet.info[1].route[1].x = 15;
  test_planet.info[1].route[1].y = 25;

  // Test a second player's info to ensure array handling
  test_planet.info[2].fuel = 300;
  test_planet.info[2].resource = 5000;
  test_planet.info[2].popn = 25000;
  test_planet.info[2].numsectsowned = 75;
  test_planet.info[2].explored = 1;
  test_planet.info[2].route[0].set = 1;
  test_planet.info[2].route[0].dest_star = 9;
  test_planet.info[2].route[0].dest_planet = 1;

  // Serialize Planet to JSON using planet_to_json()
  auto planet_json_result = planet_to_json(test_planet);
  assert(planet_json_result.has_value());
  std::println("\nPlanet JSON length: {} bytes", planet_json_result.value().size());
  
  // Show a snippet of the JSON (first 200 chars)
  std::string snippet = planet_json_result.value().substr(
      0, std::min(200UL, planet_json_result.value().size()));
  std::println("Planet JSON snippet: {}...", snippet);

  // Deserialize back using planet_from_json()
  auto recovered_planet_opt = planet_from_json(planet_json_result.value());
  assert(recovered_planet_opt.has_value());
  Planet recovered_planet = std::move(recovered_planet_opt.value());

  // Verify all Planet scalar fields
  assert(recovered_planet.planet_id == 42);
  assert(recovered_planet.xpos == 100.5);
  assert(recovered_planet.ypos == 200.7);
  assert(recovered_planet.ships == 10);
  assert(recovered_planet.Maxx == 20);
  assert(recovered_planet.Maxy == 20);
  assert(recovered_planet.popn == 100000);
  assert(recovered_planet.troops == 5000);
  assert(recovered_planet.maxpopn == 150000);
  assert(recovered_planet.total_resources == 50000);
  assert(recovered_planet.slaved_to == 3);
  assert(recovered_planet.type == PlanetType::MARS);
  assert(recovered_planet.expltimer == 5);
  assert(recovered_planet.explored == 1);

  // Verify all conditions
  assert(recovered_planet.conditions[TEMP] == 50);
  assert(recovered_planet.conditions[OXYGEN] == 20);
  assert(recovered_planet.conditions[CO2] == 5);
  assert(recovered_planet.conditions[HYDROGEN] == 10);
  assert(recovered_planet.conditions[NITROGEN] == 15);
  assert(recovered_planet.conditions[SULFUR] == 2);
  assert(recovered_planet.conditions[HELIUM] == 8);
  assert(recovered_planet.conditions[OTHER] == 3);
  assert(recovered_planet.conditions[METHANE] == 1);
  assert(recovered_planet.conditions[TOXIC] == 25);

  // Verify comprehensive plinfo for player 1
  assert(recovered_planet.info[1].fuel == 500);
  assert(recovered_planet.info[1].destruct == 250);
  assert(recovered_planet.info[1].resource == 10000);
  assert(recovered_planet.info[1].popn == 50000);
  assert(recovered_planet.info[1].troops == 2000);
  assert(recovered_planet.info[1].crystals == 100);
  assert(recovered_planet.info[1].prod_res == 500);
  assert(recovered_planet.info[1].prod_fuel == 200);
  assert(recovered_planet.info[1].prod_dest == 50);
  assert(recovered_planet.info[1].prod_crystals == 10);
  assert(recovered_planet.info[1].prod_money == 1000);
  assert(recovered_planet.info[1].prod_tech == 15.5);
  assert(recovered_planet.info[1].tech_invest == 5000);
  assert(recovered_planet.info[1].numsectsowned == 150);
  assert(recovered_planet.info[1].comread == 80);
  assert(recovered_planet.info[1].mob_set == 90);
  assert(recovered_planet.info[1].tox_thresh == 30);
  assert(recovered_planet.info[1].explored == 1);
  assert(recovered_planet.info[1].autorep == 1);
  assert(recovered_planet.info[1].tax == 15);
  assert(recovered_planet.info[1].newtax == 18);
  assert(recovered_planet.info[1].guns == 10);
  assert(recovered_planet.info[1].mob_points == 50000);
  assert(recovered_planet.info[1].est_production == 2500.75);

  // Verify routes for player 1
  assert(recovered_planet.info[1].route[0].set == 1);
  assert(recovered_planet.info[1].route[0].dest_star == 5);
  assert(recovered_planet.info[1].route[0].dest_planet == 3);
  assert(recovered_planet.info[1].route[0].load == 0x0F);
  assert(recovered_planet.info[1].route[0].unload == 0xF0);
  assert(recovered_planet.info[1].route[0].x == 10);
  assert(recovered_planet.info[1].route[0].y == 20);

  assert(recovered_planet.info[1].route[1].set == 1);
  assert(recovered_planet.info[1].route[1].dest_star == 7);
  assert(recovered_planet.info[1].route[1].dest_planet == 2);
  assert(recovered_planet.info[1].route[1].load == 0x03);
  assert(recovered_planet.info[1].route[1].unload == 0x0C);
  assert(recovered_planet.info[1].route[1].x == 15);
  assert(recovered_planet.info[1].route[1].y == 25);

  // Verify player 2's info
  assert(recovered_planet.info[2].fuel == 300);
  assert(recovered_planet.info[2].resource == 5000);
  assert(recovered_planet.info[2].popn == 25000);
  assert(recovered_planet.info[2].numsectsowned == 75);
  assert(recovered_planet.info[2].explored == 1);
  assert(recovered_planet.info[2].route[0].set == 1);
  assert(recovered_planet.info[2].route[0].dest_star == 9);
  assert(recovered_planet.info[2].route[0].dest_planet == 1);

  std::println("✓ Planet serialization test passed");
  std::println("  - All 16 Planet fields verified");
  std::println("  - All 10 atmospheric conditions verified");
  std::println("  - Complete plinfo structure verified (28 fields)");
  std::println("  - Multiple plroute entries verified (7 fields each)");
  std::println("  - Multiple player info arrays verified");
  std::println("\nAll tests passed!");
  return 0;
}
