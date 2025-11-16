// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create JsonStore and PlanetRepository
  JsonStore store(db);
  PlanetRepository repo(store);

  // Test 1: Create and save a basic planet
  std::println("Test 1: Save and retrieve basic planet...");
  Planet planet1(PlanetType::EARTH);
  planet1.star_id() = 1;       // Star 1
  planet1.planet_order() = 2;  // Planet 2
  planet1.xpos() = 100.5;
  planet1.ypos() = 200.7;
  planet1.ships() = 10;
  planet1.Maxx() = 20;
  planet1.Maxy() = 20;
  planet1.popn() = 100000;
  planet1.troops() = 5000;
  planet1.maxpopn() = 150000;
  planet1.total_resources() = 50000;
  planet1.slaved_to() = 3;
  planet1.type() = PlanetType::MARS;
  planet1.expltimer() = 5;
  planet1.explored() = 1;

  // Save to star 1, planet 2
  assert(repo.save(planet1));

  // Retrieve and verify
  auto retrieved1 = repo.find_by_location(1, 2);
  assert(retrieved1.has_value());
  assert(retrieved1->planet_order() == 2);
  assert(retrieved1->xpos() == 100.5);
  assert(retrieved1->ypos() == 200.7);
  assert(retrieved1->ships() == 10);
  assert(retrieved1->Maxx() == 20);
  assert(retrieved1->Maxy() == 20);
  assert(retrieved1->popn() == 100000);
  assert(retrieved1->troops() == 5000);
  assert(retrieved1->maxpopn() == 150000);
  assert(retrieved1->total_resources() == 50000);
  assert(retrieved1->slaved_to() == 3);
  assert(retrieved1->type() == PlanetType::MARS);
  assert(retrieved1->expltimer() == 5);
  assert(retrieved1->explored() == 1);
  std::println("✓ Basic planet save/retrieve works");

  // Test 2: Save planet with conditions
  std::println("\nTest 2: Save planet with atmospheric conditions...");
  Planet planet2(PlanetType::ICEBALL);
  planet2.star_id() = 2;       // Star 2
  planet2.planet_order() = 1;  // Planet 1
  planet2.xpos() = 50.0;
  planet2.ypos() = 75.0;
  planet2.Maxy() = 15;
  planet2.Maxx() = 15;
  planet2.conditions(TEMP) = 50;
  planet2.conditions(OXYGEN) = 20;
  planet2.conditions(CO2) = 5;
  planet2.conditions(HYDROGEN) = 10;
  planet2.conditions(NITROGEN) = 15;
  planet2.conditions(SULFUR) = 2;
  planet2.conditions(HELIUM) = 8;
  planet2.conditions(OTHER) = 3;
  planet2.conditions(METHANE) = 1;
  planet2.conditions(TOXIC) = 25;

  assert(repo.save(planet2));

  auto retrieved2 = repo.find_by_location(2, 1);
  assert(retrieved2.has_value());
  assert(retrieved2->planet_order() == 1);
  assert(retrieved2->conditions(TEMP) == 50);
  assert(retrieved2->conditions(OXYGEN) == 20);
  assert(retrieved2->conditions(CO2) == 5);
  assert(retrieved2->conditions(HYDROGEN) == 10);
  assert(retrieved2->conditions(NITROGEN) == 15);
  assert(retrieved2->conditions(SULFUR) == 2);
  assert(retrieved2->conditions(HELIUM) == 8);
  assert(retrieved2->conditions(OTHER) == 3);
  assert(retrieved2->conditions(METHANE) == 1);
  assert(retrieved2->conditions(TOXIC) == 25);
  std::println("✓ Atmospheric conditions preserved correctly");

  // Test 3: Save planet with player info
  std::println("\nTest 3: Save planet with player info...");
  Planet planet3(PlanetType::ASTEROID);
  planet3.star_id() = 3;       // Star 3
  planet3.planet_order() = 0;  // Planet 0
  planet3.xpos() = 123.4;
  planet3.ypos() = 567.8;
  planet3.Maxx() = 25;
  planet3.Maxy() = 25;

  // Initialize plinfo for player 1
  planet3.info(1).fuel = 500;
  planet3.info(1).destruct = 250;
  planet3.info(1).resource = 10000;
  planet3.info(1).popn = 50000;
  planet3.info(1).troops = 2000;
  planet3.info(1).crystals = 100;
  planet3.info(1).prod_res = 500;
  planet3.info(1).prod_fuel = 200;
  planet3.info(1).prod_dest = 50;
  planet3.info(1).prod_crystals = 10;
  planet3.info(1).prod_money = 1000;
  planet3.info(1).prod_tech = 15.5;
  planet3.info(1).tech_invest = 5000;
  planet3.info(1).numsectsowned = 150;
  planet3.info(1).comread = 80;
  planet3.info(1).mob_set = 90;
  planet3.info(1).tox_thresh = 30;
  planet3.info(1).explored = 1;
  planet3.info(1).autorep = 1;
  planet3.info(1).tax = 15;
  planet3.info(1).newtax = 18;
  planet3.info(1).guns = 10;
  planet3.info(1).mob_points = 50000;
  planet3.info(1).est_production = 2500.75;

  assert(repo.save(planet3));

  auto retrieved3 = repo.find_by_location(3, 0);
  assert(retrieved3.has_value());
  assert(retrieved3->planet_order() == 0);
  assert(retrieved3->info(1).fuel == 500);
  assert(retrieved3->info(1).destruct == 250);
  assert(retrieved3->info(1).resource == 10000);
  assert(retrieved3->info(1).popn == 50000);
  assert(retrieved3->info(1).troops == 2000);
  assert(retrieved3->info(1).crystals == 100);
  assert(retrieved3->info(1).prod_res == 500);
  assert(retrieved3->info(1).prod_fuel == 200);
  assert(retrieved3->info(1).prod_dest == 50);
  assert(retrieved3->info(1).prod_crystals == 10);
  assert(retrieved3->info(1).prod_money == 1000);
  assert(retrieved3->info(1).prod_tech == 15.5);
  assert(retrieved3->info(1).tech_invest == 5000);
  assert(retrieved3->info(1).numsectsowned == 150);
  assert(retrieved3->info(1).comread == 80);
  assert(retrieved3->info(1).mob_set == 90);
  assert(retrieved3->info(1).tox_thresh == 30);
  assert(retrieved3->info(1).explored == 1);
  assert(retrieved3->info(1).autorep == 1);
  assert(retrieved3->info(1).tax == 15);
  assert(retrieved3->info(1).newtax == 18);
  assert(retrieved3->info(1).guns == 10);
  assert(retrieved3->info(1).mob_points == 50000);
  assert(retrieved3->info(1).est_production == 2500.75);
  std::println("✓ Player info preserved correctly");

  // Test 4: Save planet with routes
  std::println("\nTest 4: Save planet with shipping routes...");
  Planet planet4(PlanetType::EARTH);
  planet4.star_id() = 4;       // Star 4
  planet4.planet_order() = 3;  // Planet 3
  planet4.xpos() = 10.0;
  planet4.ypos() = 20.0;
  planet4.Maxx() = 30;
  planet4.Maxy() = 30;

  // Initialize routes for player 1
  planet4.info(1).route[0].set = 1;
  planet4.info(1).route[0].dest_star = 5;
  planet4.info(1).route[0].dest_planet = 3;
  planet4.info(1).route[0].load = 0x0F;
  planet4.info(1).route[0].unload = 0xF0;
  planet4.info(1).route[0].x = 10;
  planet4.info(1).route[0].y = 20;

  planet4.info(1).route[1].set = 1;
  planet4.info(1).route[1].dest_star = 7;
  planet4.info(1).route[1].dest_planet = 2;
  planet4.info(1).route[1].load = 0x03;
  planet4.info(1).route[1].unload = 0x0C;
  planet4.info(1).route[1].x = 15;
  planet4.info(1).route[1].y = 25;

  assert(repo.save(planet4));

  auto retrieved4 = repo.find_by_location(4, 3);
  assert(retrieved4.has_value());
  assert(retrieved4->planet_order() == 3);
  assert(retrieved4->info(1).route[0].set == 1);
  assert(retrieved4->info(1).route[0].dest_star == 5);
  assert(retrieved4->info(1).route[0].dest_planet == 3);
  assert(retrieved4->info(1).route[0].load == 0x0F);
  assert(retrieved4->info(1).route[0].unload == 0xF0);
  assert(retrieved4->info(1).route[0].x == 10);
  assert(retrieved4->info(1).route[0].y == 20);
  assert(retrieved4->info(1).route[1].set == 1);
  assert(retrieved4->info(1).route[1].dest_star == 7);
  assert(retrieved4->info(1).route[1].dest_planet == 2);
  assert(retrieved4->info(1).route[1].load == 0x03);
  assert(retrieved4->info(1).route[1].unload == 0x0C);
  assert(retrieved4->info(1).route[1].x == 15);
  assert(retrieved4->info(1).route[1].y == 25);
  std::println("✓ Shipping routes preserved correctly");

  // Test 5: Update existing planet
  std::println("\nTest 5: Update existing planet...");
  retrieved1->popn() = 200000;
  retrieved1->troops() = 10000;
  // star_id already set from original creation
  assert(repo.save(*retrieved1));
  
  auto updated = repo.find_by_location(1, 2);
  assert(updated.has_value());
  assert(updated->planet_order() == 2);
  assert(updated->popn() == 200000);
  assert(updated->troops() == 10000);
  std::println("✓ Planet update works correctly");

  // Test 6: Multiple planets in same star system
  std::println("\nTest 6: Multiple planets in same star...");
  Planet planet5(PlanetType::GASGIANT);
  planet5.star_id() = 5;       // Star 5
  planet5.planet_order() = 0;  // Planet 0
  planet5.xpos() = 200.0;
  planet5.ypos() = 300.0;
  planet5.Maxx() = 10;
  planet5.Maxy() = 10;

  Planet planet6(PlanetType::WATER);
  planet6.star_id() = 5;       // Star 5
  planet6.planet_order() = 1;  // Planet 1
  planet6.xpos() = 250.0;
  planet6.ypos() = 350.0;
  planet6.Maxx() = 12;
  planet6.Maxy() = 12;

  // Save both to star 5
  assert(repo.save(planet5));
  assert(repo.save(planet6));

  auto p5 = repo.find_by_location(5, 0);
  auto p6 = repo.find_by_location(5, 1);
  assert(p5.has_value());
  assert(p6.has_value());
  assert(p5->planet_order() == 0);
  assert(p6->planet_order() == 1);
  assert(p5->type() == PlanetType::GASGIANT);
  assert(p6->type() == PlanetType::WATER);
  std::println("✓ Multiple planets per star works correctly");

  // Test 7: Non-existent planet returns nullopt
  std::println("\nTest 7: Non-existent planet returns nullopt...");
  auto not_found = repo.find_by_location(99, 99);
  assert(!not_found.has_value());
  std::println("✓ Non-existent planet correctly returns nullopt");

  // Test 8: Multiple players on same planet
  std::println("\nTest 8: Multiple players on same planet...");
  Planet planet7(PlanetType::EARTH);
  planet7.star_id() = 6;       // Star 6
  planet7.planet_order() = 1;  // Planet 1
  planet7.xpos() = 111.1;
  planet7.ypos() = 222.2;
  planet7.Maxx() = 20;
  planet7.Maxy() = 20;

  planet7.info(1).fuel = 1000;
  planet7.info(1).popn = 50000;
  planet7.info(2).fuel = 500;
  planet7.info(2).popn = 30000;
  planet7.info(3).fuel = 250;
  planet7.info(3).popn = 10000;

  assert(repo.save(planet7));

  auto retrieved7 = repo.find_by_location(6, 1);
  assert(retrieved7.has_value());
  assert(retrieved7->planet_order() == 1);
  assert(retrieved7->info(1).fuel == 1000);
  assert(retrieved7->info(1).popn == 50000);
  assert(retrieved7->info(2).fuel == 500);
  assert(retrieved7->info(2).popn == 30000);
  assert(retrieved7->info(3).fuel == 250);
  assert(retrieved7->info(3).popn == 10000);
  std::println("✓ Multiple players per planet preserved correctly");

  std::println("\n✅ All PlanetRepository tests passed!");
  return 0;
}
