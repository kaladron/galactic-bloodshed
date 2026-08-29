// SPDX-License-Identifier: Apache-2.0

/// \file planet_repository_test.cc
/// \brief Unit tests for PlanetRepository CRUD operations and SQLite JSON
/// persistence.

import dallib;
import gb.entities;
import gb.repositories;
import test;
import std;

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create JsonStore and PlanetRepository
  JsonStore store(db);
  PlanetRepository repo(store);

  // Create and save a basic planet
  std::println(std::cout, "Save and retrieve basic planet...");
  Planet planet1(PlanetType::EARTH, Coordinates{20, 20});
  planet1.star_id() = 1;       // Star 1
  planet1.planet_order() = 2;  // Planet 2
  planet1.xpos() = 100.5;
  planet1.ypos() = 200.7;
  planet1.ships() = 10;
  planet1.popn() = 100000;
  planet1.troops() = 5000;
  planet1.maxpopn() = 150000;
  planet1.total_resources() = 50000;
  planet1.slaved_to() = 3;
  planet1.type() = PlanetType::MARS;
  planet1.expltimer() = 5;
  planet1.explored() = 1;

  // Save to star 1, planet 2
  test::expect_true(repo.save(planet1));

  // Retrieve and verify
  auto retrieved1 = repo.find_by_location(1, 2);
  test::expect_true(retrieved1.has_value());
  test::expect_eq(retrieved1->planet_order(), 2);
  test::expect_eq(retrieved1->xpos(), 100.5);
  test::expect_eq(retrieved1->ypos(), 200.7);
  test::expect_eq(retrieved1->ships(), 10);
  test::expect_eq(retrieved1->dimensions(), Coordinates(20, 20));
  test::expect_eq(retrieved1->popn(), 100000);
  test::expect_eq(retrieved1->troops(), 5000);
  test::expect_eq(retrieved1->maxpopn(), 150000);
  test::expect_eq(retrieved1->total_resources(), 50000);
  test::expect_eq(retrieved1->slaved_to(), 3);
  test::expect_eq(retrieved1->type(), PlanetType::MARS);
  test::expect_eq(retrieved1->expltimer(), 5);
  test::expect_eq(retrieved1->explored(), 1);
  std::println(std::cout, "✓ Basic planet save/retrieve works");

  // Save planet with conditions
  std::println(std::cout,
               "\nTest 2: Save planet with atmospheric conditions...");
  Planet planet2(PlanetType::ICEBALL, Coordinates{15, 15});
  planet2.star_id() = 2;       // Star 2
  planet2.planet_order() = 1;  // Planet 1
  planet2.xpos() = 50.0;
  planet2.ypos() = 75.0;
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

  test::expect_true(repo.save(planet2));

  auto retrieved2 = repo.find_by_location(2, 1);
  test::expect_true(retrieved2.has_value());
  test::expect_eq(retrieved2->planet_order(), 1);
  test::expect_eq(retrieved2->conditions(TEMP), 50);
  test::expect_eq(retrieved2->conditions(OXYGEN), 20);
  test::expect_eq(retrieved2->conditions(CO2), 5);
  test::expect_eq(retrieved2->conditions(HYDROGEN), 10);
  test::expect_eq(retrieved2->conditions(NITROGEN), 15);
  test::expect_eq(retrieved2->conditions(SULFUR), 2);
  test::expect_eq(retrieved2->conditions(HELIUM), 8);
  test::expect_eq(retrieved2->conditions(OTHER), 3);
  test::expect_eq(retrieved2->conditions(METHANE), 1);
  test::expect_eq(retrieved2->conditions(TOXIC), 25);
  std::println(std::cout, "✓ Atmospheric conditions preserved correctly");

  // Save planet with player info
  std::println(std::cout, "\nTest 3: Save planet with player info...");
  Planet planet3(PlanetType::ASTEROID, Coordinates{25, 25});
  planet3.star_id() = 3;       // Star 3
  planet3.planet_order() = 0;  // Planet 0
  planet3.xpos() = 123.4;
  planet3.ypos() = 567.8;

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

  test::expect_true(repo.save(planet3));

  auto retrieved3 = repo.find_by_location(3, 0);
  test::expect_true(retrieved3.has_value());
  test::expect_eq(retrieved3->planet_order(), 0);
  test::expect_eq(retrieved3->info(1).fuel, 500);
  test::expect_eq(retrieved3->info(1).destruct, 250);
  test::expect_eq(retrieved3->info(1).resource, 10000);
  test::expect_eq(retrieved3->info(1).popn, 50000);
  test::expect_eq(retrieved3->info(1).troops, 2000);
  test::expect_eq(retrieved3->info(1).crystals, 100);
  test::expect_eq(retrieved3->info(1).prod_res, 500);
  test::expect_eq(retrieved3->info(1).prod_fuel, 200);
  test::expect_eq(retrieved3->info(1).prod_dest, 50);
  test::expect_eq(retrieved3->info(1).prod_crystals, 10);
  test::expect_eq(retrieved3->info(1).prod_money, 1000);
  test::expect_eq(retrieved3->info(1).prod_tech, 15.5);
  test::expect_eq(retrieved3->info(1).tech_invest, 5000);
  test::expect_eq(retrieved3->info(1).numsectsowned, 150);
  test::expect_eq(retrieved3->info(1).comread, 80);
  test::expect_eq(retrieved3->info(1).mob_set, 90);
  test::expect_eq(retrieved3->info(1).tox_thresh,
                  std::optional<std::uint32_t>{30});
  test::expect_eq(retrieved3->info(1).explored, true);
  test::expect_eq(retrieved3->info(1).autorep, 1);
  test::expect_eq(retrieved3->info(1).tax, 15);
  test::expect_eq(retrieved3->info(1).newtax, 18);
  test::expect_eq(retrieved3->info(1).guns, 10);
  test::expect_eq(retrieved3->info(1).mob_points, 50000);
  test::expect_eq(retrieved3->info(1).est_production, 2500.75);
  std::println(std::cout, "✓ Player info preserved correctly");

  // Save planet with routes
  std::println(std::cout, "\nTest 4: Save planet with shipping routes...");
  Planet planet4(PlanetType::EARTH, Coordinates{30, 30});
  planet4.star_id() = 4;       // Star 4
  planet4.planet_order() = 3;  // Planet 3
  planet4.xpos() = 10.0;
  planet4.ypos() = 20.0;

  // Initialize routes for player 1
  planet4.info(1).route[0].set = true;
  planet4.info(1).route[0].dest_star = 5;
  planet4.info(1).route[0].dest_planet = 3;
  planet4.info(1).route[0].load = CommodityManifest{
      .fuel = true, .destruct = true, .resources = true, .crystals = true};
  planet4.info(1).route[0].unload = CommodityManifest{};
  planet4.info(1).route[0].dest_coords = {10, 20};

  planet4.info(1).route[1].set = true;
  planet4.info(1).route[1].dest_star = 7;
  planet4.info(1).route[1].dest_planet = 2;
  planet4.info(1).route[1].load =
      CommodityManifest{.fuel = true, .destruct = true};
  planet4.info(1).route[1].unload =
      CommodityManifest{.resources = true, .crystals = true};
  planet4.info(1).route[1].dest_coords = {15, 25};

  test::expect_true(repo.save(planet4));

  auto retrieved4 = repo.find_by_location(4, 3);
  test::expect_true(retrieved4.has_value());
  test::expect_eq(retrieved4->planet_order(), 3);
  test::expect_true(retrieved4->info(1).route[0].set);
  test::expect_eq(retrieved4->info(1).route[0].dest_star, starnum_t{5});
  test::expect_eq(retrieved4->info(1).route[0].dest_planet, planetnum_t{3});
  test::expect_eq(retrieved4->info(1).route[0].load,
                  planet4.info(1).route[0].load);
  test::expect_eq(retrieved4->info(1).route[0].unload,
                  planet4.info(1).route[0].unload);
  test::expect_eq(retrieved4->info(1).route[0].dest_coords,
                  (Coordinates{10, 20}));
  test::expect_true(retrieved4->info(1).route[1].set);
  test::expect_eq(retrieved4->info(1).route[1].dest_star, starnum_t{7});
  test::expect_eq(retrieved4->info(1).route[1].dest_planet, planetnum_t{2});
  test::expect_eq(retrieved4->info(1).route[1].load,
                  planet4.info(1).route[1].load);
  test::expect_eq(retrieved4->info(1).route[1].unload,
                  planet4.info(1).route[1].unload);
  test::expect_eq(retrieved4->info(1).route[1].dest_coords,
                  (Coordinates{15, 25}));
  std::println(std::cout, "✓ Shipping routes preserved correctly");

  // Update existing planet
  std::println(std::cout, "\nTest 5: Update existing planet...");
  retrieved1->popn() = 200000;
  retrieved1->troops() = 10000;
  test::expect_true(repo.save(*retrieved1));

  auto updated = repo.find_by_location(1, 2);
  test::expect_true(updated.has_value());
  test::expect_eq(updated->planet_order(), 2);
  test::expect_eq(updated->popn(), 200000);
  test::expect_eq(updated->troops(), 10000);
  std::println(std::cout, "✓ Planet update works correctly");

  // Multiple planets in same star system
  std::println(std::cout, "\nTest 6: Multiple planets in same star...");
  Planet planet5(PlanetType::GASGIANT, Coordinates{10, 10});
  planet5.star_id() = 5;       // Star 5
  planet5.planet_order() = 0;  // Planet 0
  planet5.xpos() = 200.0;
  planet5.ypos() = 300.0;

  Planet planet6(PlanetType::WATER, Coordinates{12, 12});
  planet6.star_id() = 5;       // Star 5
  planet6.planet_order() = 1;  // Planet 1
  planet6.xpos() = 250.0;
  planet6.ypos() = 350.0;

  // Save both to star 5
  test::expect_true(repo.save(planet5));
  test::expect_true(repo.save(planet6));

  auto p5 = repo.find_by_location(5, 0);
  auto p6 = repo.find_by_location(5, 1);
  test::expect_true(p5.has_value());
  test::expect_true(p6.has_value());
  test::expect_eq(p5->planet_order(), 0);
  test::expect_eq(p6->planet_order(), 1);
  test::expect_eq(p5->type(), PlanetType::GASGIANT);
  test::expect_eq(p6->type(), PlanetType::WATER);
  std::println(std::cout, "✓ Multiple planets per star works correctly");

  // Non-existent planet returns nullopt
  std::println(std::cout, "\nTest 7: Non-existent planet returns nullopt...");
  auto not_found = repo.find_by_location(99, 99);
  test::expect_false(not_found.has_value());
  std::println(std::cout, "✓ Non-existent planet correctly returns nullopt");

  // Multiple players on same planet
  std::println(std::cout, "\nTest 8: Multiple players on same planet...");
  Planet planet7(PlanetType::EARTH, Coordinates{20, 20});
  planet7.star_id() = 6;       // Star 6
  planet7.planet_order() = 1;  // Planet 1
  planet7.xpos() = 111.1;
  planet7.ypos() = 222.2;

  planet7.info(1).fuel = 1000;
  planet7.info(1).popn = 50000;
  planet7.info(2).fuel = 500;
  planet7.info(2).popn = 30000;
  planet7.info(3).fuel = 250;
  planet7.info(3).popn = 10000;

  test::expect_true(repo.save(planet7));

  auto retrieved7 = repo.find_by_location(6, 1);
  test::expect_true(retrieved7.has_value());
  test::expect_eq(retrieved7->planet_order(), 1);
  test::expect_eq(retrieved7->info(1).fuel, 1000);
  test::expect_eq(retrieved7->info(1).popn, 50000);
  test::expect_eq(retrieved7->info(2).fuel, 500);
  test::expect_eq(retrieved7->info(2).popn, 30000);
  test::expect_eq(retrieved7->info(3).fuel, 250);
  test::expect_eq(retrieved7->info(3).popn, 10000);
  std::println(std::cout, "✓ Multiple players per planet preserved correctly");

  std::println(std::cout, "\n✅ All PlanetRepository tests passed!");
  return 0;
}
