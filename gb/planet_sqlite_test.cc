// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import std.compat;

#include <sqlite3.h>

#include <cassert>
#include <cstring>

int main() {
  // CRITICAL: Always create in-memory database BEFORE calling
  // initialize_schema()
  Database db(":memory:");

  // Initialize database tables - this creates all required tables
  initialize_schema(db);

  // Create a test Planet
  Planet test_planet(PlanetType::EARTH);

  // Initialize scalar fields
  test_planet.star_id() = 1;  // Match the star we'll create below
  test_planet.planet_order() = 2;
  test_planet.xpos() = 100.5;
  test_planet.ypos() = 200.7;
  test_planet.ships() = 10;
  test_planet.Maxx() = 20;
  test_planet.Maxy() = 20;
  test_planet.popn() = 100000;
  test_planet.troops() = 5000;
  test_planet.maxpopn() = 150000;
  test_planet.total_resources() = 50000;
  test_planet.slaved_to() = 3;
  test_planet.type() = PlanetType::MARS;
  test_planet.expltimer() = 5;
  test_planet.explored() = 1;

  // Initialize conditions
  test_planet.conditions(TEMP) = 50;
  test_planet.conditions(OXYGEN) = 20;
  test_planet.conditions(CO2) = 5;
  test_planet.conditions(HYDROGEN) = 10;
  test_planet.conditions(NITROGEN) = 15;
  test_planet.conditions(SULFUR) = 2;
  test_planet.conditions(HELIUM) = 8;
  test_planet.conditions(OTHER) = 3;
  test_planet.conditions(METHANE) = 1;
  test_planet.conditions(TOXIC) = 25;

  // Initialize plinfo for player 1
  test_planet.info(1).fuel = 500;
  test_planet.info(1).destruct = 250;
  test_planet.info(1).resource = 10000;
  test_planet.info(1).popn = 50000;
  test_planet.info(1).troops = 2000;
  test_planet.info(1).crystals = 100;
  test_planet.info(1).prod_res = 500;
  test_planet.info(1).prod_fuel = 200;
  test_planet.info(1).prod_dest = 50;
  test_planet.info(1).prod_crystals = 10;
  test_planet.info(1).prod_money = 1000;
  test_planet.info(1).prod_tech = 15.5;
  test_planet.info(1).tech_invest = 5000;
  test_planet.info(1).numsectsowned = 150;
  test_planet.info(1).comread = 80;
  test_planet.info(1).mob_set = 90;
  test_planet.info(1).tox_thresh = 30;
  test_planet.info(1).explored = 1;
  test_planet.info(1).autorep = 1;
  test_planet.info(1).tax = 15;
  test_planet.info(1).newtax = 18;
  test_planet.info(1).guns = 10;
  test_planet.info(1).mob_points = 50000;
  test_planet.info(1).est_production = 2500.75;

  // Initialize routes for player 1
  test_planet.info(1).route[0].set = 1;
  test_planet.info(1).route[0].dest_star = 5;
  test_planet.info(1).route[0].dest_planet = 3;
  test_planet.info(1).route[0].load = 0x0F;
  test_planet.info(1).route[0].unload = 0xF0;
  test_planet.info(1).route[0].x = 10;
  test_planet.info(1).route[0].y = 20;

  test_planet.info(1).route[1].set = 1;
  test_planet.info(1).route[1].dest_star = 7;
  test_planet.info(1).route[1].dest_planet = 2;
  test_planet.info(1).route[1].load = 0x03;
  test_planet.info(1).route[1].unload = 0x0C;
  test_planet.info(1).route[1].x = 15;
  test_planet.info(1).route[1].y = 25;

  // Initialize plinfo for player 2 (to test multiple players)
  test_planet.info(2).fuel = 300;
  test_planet.info(2).destruct = 150;
  test_planet.info(2).resource = 5000;
  test_planet.info(2).popn = 25000;
  test_planet.info(2).troops = 1000;
  test_planet.info(2).crystals = 50;

  // Use Repository to create new objects - this is the DAL layer
  JsonStore store(db);

  // Create a test Star (needed for planet storage)
  star_struct test_star_data{};
  test_star_data.star_id = 1;
  test_star_data.name = "TestStar";
  // Initialize with 5 empty planet names
  for (int i = 0; i < 5; i++) {
    test_star_data.pnames.push_back("");
  }
  Star test_star(test_star_data);

  // Save star using repository
  StarRepository star_repo(store);
  star_repo.save(test_star);

  // Save planet using repository
  PlanetRepository planet_repo(store);
  planet_repo.save(test_planet);

  // Create EntityManager to test retrieval
  EntityManager em(db);

  // Test EntityManager peek - reads from SQLite
  const auto* retrieved_ptr =
      em.peek_planet(1, 2);  // star_id = 1, planet_order = 2
  assert(retrieved_ptr != nullptr);
  const Planet& retrieved = *retrieved_ptr;

  // Verify scalar fields
  assert(retrieved.star_id() == test_planet.star_id());
  assert(retrieved.planet_order() == test_planet.planet_order());
  assert(retrieved.xpos() == test_planet.xpos());
  assert(retrieved.ypos() == test_planet.ypos());
  assert(retrieved.ships() == test_planet.ships());
  assert(retrieved.Maxx() == test_planet.Maxx());
  assert(retrieved.Maxy() == test_planet.Maxy());
  assert(retrieved.popn() == test_planet.popn());
  assert(retrieved.troops() == test_planet.troops());
  assert(retrieved.maxpopn() == test_planet.maxpopn());
  assert(retrieved.total_resources() == test_planet.total_resources());
  assert(retrieved.slaved_to() == test_planet.slaved_to());
  assert(retrieved.type() == test_planet.type());
  assert(retrieved.expltimer() == test_planet.expltimer());
  assert(retrieved.explored() == test_planet.explored());

  // Verify conditions
  assert(retrieved.conditions(TEMP) == test_planet.conditions(TEMP));
  assert(retrieved.conditions(OXYGEN) == test_planet.conditions(OXYGEN));
  assert(retrieved.conditions(CO2) == test_planet.conditions(CO2));
  assert(retrieved.conditions(HYDROGEN) == test_planet.conditions(HYDROGEN));
  assert(retrieved.conditions(NITROGEN) == test_planet.conditions(NITROGEN));
  assert(retrieved.conditions(SULFUR) == test_planet.conditions(SULFUR));
  assert(retrieved.conditions(HELIUM) == test_planet.conditions(HELIUM));
  assert(retrieved.conditions(OTHER) == test_planet.conditions(OTHER));
  assert(retrieved.conditions(METHANE) == test_planet.conditions(METHANE));
  assert(retrieved.conditions(TOXIC) == test_planet.conditions(TOXIC));

  // Verify plinfo for player 1
  assert(retrieved.info(1).fuel == test_planet.info(1).fuel);
  assert(retrieved.info(1).destruct == test_planet.info(1).destruct);
  assert(retrieved.info(1).resource == test_planet.info(1).resource);
  assert(retrieved.info(1).popn == test_planet.info(1).popn);
  assert(retrieved.info(1).troops == test_planet.info(1).troops);
  assert(retrieved.info(1).crystals == test_planet.info(1).crystals);
  assert(retrieved.info(1).prod_res == test_planet.info(1).prod_res);
  assert(retrieved.info(1).prod_fuel == test_planet.info(1).prod_fuel);
  assert(retrieved.info(1).prod_dest == test_planet.info(1).prod_dest);
  assert(retrieved.info(1).prod_crystals == test_planet.info(1).prod_crystals);
  assert(retrieved.info(1).prod_money == test_planet.info(1).prod_money);
  assert(retrieved.info(1).prod_tech == test_planet.info(1).prod_tech);
  assert(retrieved.info(1).tech_invest == test_planet.info(1).tech_invest);
  assert(retrieved.info(1).numsectsowned == test_planet.info(1).numsectsowned);
  assert(retrieved.info(1).comread == test_planet.info(1).comread);
  assert(retrieved.info(1).mob_set == test_planet.info(1).mob_set);
  assert(retrieved.info(1).tox_thresh == test_planet.info(1).tox_thresh);
  assert(retrieved.info(1).explored == test_planet.info(1).explored);
  assert(retrieved.info(1).autorep == test_planet.info(1).autorep);
  assert(retrieved.info(1).tax == test_planet.info(1).tax);
  assert(retrieved.info(1).newtax == test_planet.info(1).newtax);
  assert(retrieved.info(1).guns == test_planet.info(1).guns);
  assert(retrieved.info(1).mob_points == test_planet.info(1).mob_points);
  assert(retrieved.info(1).est_production ==
         test_planet.info(1).est_production);

  // Verify routes for player 1
  assert(retrieved.info(1).route[0].set == test_planet.info(1).route[0].set);
  assert(retrieved.info(1).route[0].dest_star ==
         test_planet.info(1).route[0].dest_star);
  assert(retrieved.info(1).route[0].dest_planet ==
         test_planet.info(1).route[0].dest_planet);
  assert(retrieved.info(1).route[0].load == test_planet.info(1).route[0].load);
  assert(retrieved.info(1).route[0].unload ==
         test_planet.info(1).route[0].unload);
  assert(retrieved.info(1).route[0].x == test_planet.info(1).route[0].x);
  assert(retrieved.info(1).route[0].y == test_planet.info(1).route[0].y);

  assert(retrieved.info(1).route[1].set == test_planet.info(1).route[1].set);
  assert(retrieved.info(1).route[1].dest_star ==
         test_planet.info(1).route[1].dest_star);
  assert(retrieved.info(1).route[1].dest_planet ==
         test_planet.info(1).route[1].dest_planet);
  assert(retrieved.info(1).route[1].load == test_planet.info(1).route[1].load);
  assert(retrieved.info(1).route[1].unload ==
         test_planet.info(1).route[1].unload);
  assert(retrieved.info(1).route[1].x == test_planet.info(1).route[1].x);
  assert(retrieved.info(1).route[1].y == test_planet.info(1).route[1].y);

  // Verify plinfo for player 2
  assert(retrieved.info(2).fuel == test_planet.info(2).fuel);
  assert(retrieved.info(2).destruct == test_planet.info(2).destruct);
  assert(retrieved.info(2).resource == test_planet.info(2).resource);
  assert(retrieved.info(2).popn == test_planet.info(2).popn);
  assert(retrieved.info(2).troops == test_planet.info(2).troops);
  assert(retrieved.info(2).crystals == test_planet.info(2).crystals);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println("Planet SQLite storage test passed!");
  return 0;
}
