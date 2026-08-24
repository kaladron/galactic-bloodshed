// SPDX-License-Identifier: Apache-2.0

/// \file planet_sqlite_test.cc
/// \brief Unit tests for Planet SQLite table persistence and round-trip
/// verification.

import dallib;
import gblib;
import test;
import std;

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
  test_planet.info(1).route[0].dest_coords = {10, 20};

  test_planet.info(1).route[1].set = 1;
  test_planet.info(1).route[1].dest_star = 7;
  test_planet.info(1).route[1].dest_planet = 2;
  test_planet.info(1).route[1].load = 0x03;
  test_planet.info(1).route[1].unload = 0x0C;
  test_planet.info(1).route[1].dest_coords = {15, 25};

  // Initialize plinfo for player 2 (to test multiple players)
  test_planet.info(2).fuel = 300;
  test_planet.info(2).destruct = 150;
  test_planet.info(2).resource = 5000;
  test_planet.info(2).popn = 20000;
  test_planet.info(2).troops = 1000;
  test_planet.info(2).crystals = 20;

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
  test::expect_ne(retrieved_ptr, nullptr);
  const Planet& retrieved = *retrieved_ptr;

  // Verify scalar fields
  test::expect_eq(retrieved.star_id(), test_planet.star_id());
  test::expect_eq(retrieved.planet_order(), test_planet.planet_order());
  test::expect_eq(retrieved.xpos(), test_planet.xpos());
  test::expect_eq(retrieved.ypos(), test_planet.ypos());
  test::expect_eq(retrieved.ships(), test_planet.ships());
  test::expect_eq(retrieved.Maxx(), test_planet.Maxx());
  test::expect_eq(retrieved.Maxy(), test_planet.Maxy());
  test::expect_eq(retrieved.popn(), test_planet.popn());
  test::expect_eq(retrieved.troops(), test_planet.troops());
  test::expect_eq(retrieved.maxpopn(), test_planet.maxpopn());
  test::expect_eq(retrieved.total_resources(), test_planet.total_resources());
  test::expect_eq(retrieved.slaved_to(), test_planet.slaved_to());
  test::expect_eq(retrieved.type(), test_planet.type());
  test::expect_eq(retrieved.expltimer(), test_planet.expltimer());
  test::expect_eq(retrieved.explored(), test_planet.explored());

  // Verify conditions
  test::expect_eq(retrieved.conditions(TEMP), test_planet.conditions(TEMP));
  test::expect_eq(retrieved.conditions(OXYGEN), test_planet.conditions(OXYGEN));
  test::expect_eq(retrieved.conditions(CO2), test_planet.conditions(CO2));
  test::expect_eq(retrieved.conditions(HYDROGEN),
                  test_planet.conditions(HYDROGEN));
  test::expect_eq(retrieved.conditions(NITROGEN),
                  test_planet.conditions(NITROGEN));
  test::expect_eq(retrieved.conditions(SULFUR), test_planet.conditions(SULFUR));
  test::expect_eq(retrieved.conditions(HELIUM), test_planet.conditions(HELIUM));
  test::expect_eq(retrieved.conditions(OTHER), test_planet.conditions(OTHER));
  test::expect_eq(retrieved.conditions(METHANE),
                  test_planet.conditions(METHANE));
  test::expect_eq(retrieved.conditions(TOXIC), test_planet.conditions(TOXIC));

  // Verify plinfo for player 1
  test::expect_eq(retrieved.info(1).fuel, test_planet.info(1).fuel);
  test::expect_eq(retrieved.info(1).destruct, test_planet.info(1).destruct);
  test::expect_eq(retrieved.info(1).resource, test_planet.info(1).resource);
  test::expect_eq(retrieved.info(1).popn, test_planet.info(1).popn);
  test::expect_eq(retrieved.info(1).troops, test_planet.info(1).troops);
  test::expect_eq(retrieved.info(1).crystals, test_planet.info(1).crystals);
  test::expect_eq(retrieved.info(1).prod_res, test_planet.info(1).prod_res);
  test::expect_eq(retrieved.info(1).prod_fuel, test_planet.info(1).prod_fuel);
  test::expect_eq(retrieved.info(1).prod_dest, test_planet.info(1).prod_dest);
  test::expect_eq(retrieved.info(1).prod_crystals,
                  test_planet.info(1).prod_crystals);
  test::expect_eq(retrieved.info(1).prod_money, test_planet.info(1).prod_money);
  test::expect_eq(retrieved.info(1).prod_tech, test_planet.info(1).prod_tech);
  test::expect_eq(retrieved.info(1).tech_invest,
                  test_planet.info(1).tech_invest);
  test::expect_eq(retrieved.info(1).numsectsowned,
                  test_planet.info(1).numsectsowned);
  test::expect_eq(retrieved.info(1).comread, test_planet.info(1).comread);
  test::expect_eq(retrieved.info(1).mob_set, test_planet.info(1).mob_set);
  test::expect_eq(retrieved.info(1).tox_thresh, test_planet.info(1).tox_thresh);
  test::expect_eq(retrieved.info(1).explored, test_planet.info(1).explored);
  test::expect_eq(retrieved.info(1).autorep, test_planet.info(1).autorep);
  test::expect_eq(retrieved.info(1).tax, test_planet.info(1).tax);
  test::expect_eq(retrieved.info(1).newtax, test_planet.info(1).newtax);
  test::expect_eq(retrieved.info(1).guns, test_planet.info(1).guns);
  test::expect_eq(retrieved.info(1).mob_points, test_planet.info(1).mob_points);
  test::expect_eq(retrieved.info(1).est_production,
                  test_planet.info(1).est_production);

  // Verify routes for player 1
  test::expect_eq(retrieved.info(1).route[0].set,
                  test_planet.info(1).route[0].set);
  test::expect_eq(retrieved.info(1).route[0].dest_star,
                  test_planet.info(1).route[0].dest_star);
  test::expect_eq(retrieved.info(1).route[0].dest_planet,
                  test_planet.info(1).route[0].dest_planet);
  test::expect_eq(retrieved.info(1).route[0].load,
                  test_planet.info(1).route[0].load);
  test::expect_eq(retrieved.info(1).route[0].unload,
                  test_planet.info(1).route[0].unload);
  test::expect_eq(retrieved.info(1).route[0].dest_coords,
                  test_planet.info(1).route[0].dest_coords);

  test::expect_eq(retrieved.info(1).route[1].set,
                  test_planet.info(1).route[1].set);
  test::expect_eq(retrieved.info(1).route[1].dest_star,
                  test_planet.info(1).route[1].dest_star);
  test::expect_eq(retrieved.info(1).route[1].dest_planet,
                  test_planet.info(1).route[1].dest_planet);
  test::expect_eq(retrieved.info(1).route[1].load,
                  test_planet.info(1).route[1].load);
  test::expect_eq(retrieved.info(1).route[1].unload,
                  test_planet.info(1).route[1].unload);
  test::expect_eq(retrieved.info(1).route[1].dest_coords,
                  test_planet.info(1).route[1].dest_coords);

  // Verify plinfo for player 2
  test::expect_eq(retrieved.info(2).fuel, test_planet.info(2).fuel);
  test::expect_eq(retrieved.info(2).destruct, test_planet.info(2).destruct);
  test::expect_eq(retrieved.info(2).resource, test_planet.info(2).resource);
  test::expect_eq(retrieved.info(2).popn, test_planet.info(2).popn);
  test::expect_eq(retrieved.info(2).troops, test_planet.info(2).troops);
  test::expect_eq(retrieved.info(2).crystals, test_planet.info(2).crystals);

  // Database connection will be cleaned up automatically by Database destructor

  std::println(std::cout, "Planet SQLite storage test passed!");
  return 0;
}
