// SPDX-License-Identifier: Apache-2.0

/// \file ship_repository_test.cc
/// \brief Unit tests for ShipRepository CRUD operations and SQLite JSON
/// persistence.

import dallib;
import gblib;
import test;
import std;

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create JsonStore and ShipRepository
  JsonStore store(db);
  ShipRepository repo(store);

  // Create a test ship using ship_struct (POD, copyable)
  ship_struct test_data{};
  test_data.number = 1;
  test_data.owner = 2;
  test_data.governor = 0;
  test_data.name = "USS Enterprise";
  test_data.shipclass = "Cruiser";
  test_data.race = 2;
  test_data.xpos = 100.5;
  test_data.ypos = 200.7;
  test_data.fuel = 5000.0;
  test_data.mass = 1500.0;
  test_data.armor = 250;
  test_data.size = 1000;
  test_data.max_crew = 500;
  test_data.max_resource = 2000;
  test_data.max_destruct = 1000;
  test_data.max_fuel = 10000;
  test_data.max_speed = 9;
  test_data.build_type = ShipType::STYPE_CRUISER;
  test_data.build_cost = 50000;
  test_data.base_mass = 1200.0;
  test_data.tech = 25.5;
  test_data.complexity = 30;
  test_data.destruct = 500;
  test_data.resource = 1000;
  test_data.popn = 250;
  test_data.troops = 100;
  test_data.crystals = 50;
  test_data.damage = 0;
  test_data.rad = 0;
  test_data.type = ShipType::STYPE_CRUISER;
  test_data.speed = 5;
  test_data.active = true;
  test_data.alive = true;
  test_data.mode = false;
  test_data.bombard = false;
  test_data.mounted = false;
  test_data.cloaked = false;
  test_data.docked = false;
  test_data.guns = 1;     // Light guns
  test_data.primary = 0;  // No primary weapon
  test_data.primtype = GTYPE_NONE;
  test_data.secondary = 0;  // No secondary weapon
  test_data.sectype = GTYPE_NONE;

  // Wrap in Ship for saving
  Ship test_ship(test_data);

  // Save ship
  std::println(std::cout, "Save ship...");
  bool saved = repo.save(test_ship);
  test::expect_true(saved, "Failed to save ship");
  std::println(std::cout, "  ✓ Ship saved successfully");

  // Retrieve by ship number
  std::println(std::cout, "Retrieve ship by number...");
  auto retrieved = repo.find_by_number(1);
  test::expect_true(retrieved.has_value(), "Failed to retrieve ship");
  std::println(std::cout, "  ✓ Ship retrieved successfully");

  // Verify data integrity
  std::println(std::cout, "Verify data integrity...");
  test::expect_eq(retrieved->number(), test_ship.number());
  test::expect_eq(retrieved->owner(), test_ship.owner());
  test::expect_eq(retrieved->governor(), test_ship.governor());
  test::expect_eq(retrieved->name(), test_ship.name());
  test::expect_eq(retrieved->shipclass(), test_ship.shipclass());
  test::expect_eq(retrieved->race(), test_ship.race());
  test::expect_eq(retrieved->xpos(), test_ship.xpos());
  test::expect_eq(retrieved->ypos(), test_ship.ypos());
  test::expect_eq(retrieved->fuel(), test_ship.fuel());
  test::expect_eq(retrieved->mass(), test_ship.mass());
  test::expect_eq(retrieved->armor(), test_ship.armor());
  test::expect_eq(retrieved->size(), test_ship.size());
  test::expect_eq(retrieved->max_crew(), test_ship.max_crew());
  test::expect_eq(retrieved->tech(), test_ship.tech());
  test::expect_eq(retrieved->type(), test_ship.type());
  test::expect_eq(retrieved->active(), test_ship.active());
  test::expect_eq(retrieved->alive(), test_ship.alive());
  std::println(std::cout, "  ✓ All fields match original");

  // Update ship
  std::println(std::cout, "Update ship...");
  retrieved->fuel() = 3000.0;
  retrieved->damage() = 50;
  retrieved->xpos() = 150.0;
  saved = repo.save(*retrieved);
  test::expect_true(saved, "Failed to update ship");
  std::println(std::cout, "  ✓ Ship updated successfully");

  // Retrieve updated ship
  std::println(std::cout, "Retrieve updated ship...");
  auto updated = repo.find_by_number(1);
  test::expect_true(updated.has_value(), "Failed to retrieve updated ship");
  test::expect_eq(updated->fuel(), 3000.0);
  test::expect_eq(updated->damage(), 50);
  test::expect_eq(updated->xpos(), 150.0);
  std::println(std::cout, "  ✓ Updated values verified");

  // Save multiple ships (use ship_struct which is copyable)
  std::println(std::cout, "Save multiple ships...");
  ship_struct ship2_data = test_data;  // Copy the POD struct
  ship2_data.number = 2;
  ship2_data.name = "USS Defiant";
  Ship ship2(ship2_data);
  repo.save(ship2);

  ship_struct ship3_data = test_data;  // Copy the POD struct
  ship3_data.number = 5;               // Gap at 3 and 4
  ship3_data.name = "USS Voyager";
  Ship ship3(ship3_data);
  repo.save(ship3);

  std::println(std::cout, "  ✓ Multiple ships saved");

  // Count all ships
  std::println(std::cout, "Count all ships...");
  shipnum_t count = repo.count_all_ships();
  test::expect_eq(count, 3, "Should have 3 ships");
  std::println(std::cout, "  ✓ Ship count correct: {}", count);

  // Next available ship number (should find gap at 3)
  std::println(std::cout, "Next available ship number...");
  shipnum_t next_id = repo.next_ship_number();
  test::expect_eq(next_id, 3, "Should return 3 (first gap)");
  std::println(std::cout, "  ✓ Next ship number is: {}", next_id);

  // Delete a ship
  std::println(std::cout, "Delete ship...");
  repo.delete_ship(2);
  auto deleted = repo.find_by_number(2);
  test::expect_false(deleted.has_value(), "Ship should be deleted");
  std::println(std::cout, "  ✓ Ship deleted successfully");

  // Count after deletion
  std::println(std::cout, "Count after deletion...");
  count = repo.count_all_ships();
  test::expect_eq(count, 2, "Should have 2 ships after deletion");
  std::println(std::cout, "  ✓ Ship count correct after deletion: {}", count);

  // Find non-existent ship
  std::println(std::cout, "Find non-existent ship...");
  auto not_found = repo.find_by_number(999);
  test::expect_false(not_found.has_value(),
                     "Should not find non-existent ship");
  std::println(std::cout,
               "  ✓ Correctly returns nullopt for non-existent ship");

  // List ships
  std::println(std::cout, "List ships...");
  auto ship_ids = repo.list_ids();
  test::expect_eq(ship_ids.size(), 2);
  test::expect_eq(ship_ids[0], 1);
  test::expect_eq(ship_ids[1], 5);
  std::println(std::cout, "  ✓ list_ids returns active ship IDs in order");

  // Spatial and indexed queries
  std::println(std::cout, "Testing spatial and indexed queries...");
  {
    // Clear out earlier test ships
    repo.delete_ship(1);
    repo.delete_ship(5);

    // Setup test fleet:
    // Ship 1: Owner 1, Star 1, LEVEL_STAR, alive = true
    ship_struct s1{};
    s1.number = 1;
    s1.owner = 1;
    s1.storbits = 1;
    s1.whatorbits = ScopeLevel::LEVEL_STAR;
    s1.alive = true;
    repo.save(Ship(s1));

    // Ship 2: Owner 1, Star 1, LEVEL_STAR, alive = false (dead)
    ship_struct s2{};
    s2.number = 2;
    s2.owner = 1;
    s2.storbits = 1;
    s2.whatorbits = ScopeLevel::LEVEL_STAR;
    s2.alive = false;
    repo.save(Ship(s2));

    // Ship 3: Owner 2, Star 1, Planet 2, LEVEL_PLAN, alive = true
    ship_struct s3{};
    s3.number = 3;
    s3.owner = 2;
    s3.storbits = 1;
    s3.pnumorbits = 2;
    s3.whatorbits = ScopeLevel::LEVEL_PLAN;
    s3.alive = true;
    repo.save(Ship(s3));

    // Ship 4: Owner 2, Star 1, Planet 2, LEVEL_PLAN, alive = false (dead)
    ship_struct s4{};
    s4.number = 4;
    s4.owner = 2;
    s4.storbits = 1;
    s4.pnumorbits = 2;
    s4.whatorbits = ScopeLevel::LEVEL_PLAN;
    s4.alive = false;
    repo.save(Ship(s4));

    // Ship 5: Owner 1, Carrier Hangar (destshipno = 1), LEVEL_SHIP, alive =
    // true
    ship_struct s5{};
    s5.number = 5;
    s5.owner = 1;
    s5.destshipno = 1;
    s5.whatorbits = ScopeLevel::LEVEL_SHIP;
    s5.alive = true;
    repo.save(Ship(s5));

    // Test find_in_star
    auto star1_alive = repo.find_in_star(starnum_t{1}, true);
    test::expect_eq(star1_alive.size(), 1);
    test::expect_eq(star1_alive[0], 1);

    auto star1_all = repo.find_in_star(starnum_t{1}, false);
    test::expect_eq(star1_all.size(), 2);
    test::expect_eq(star1_all[0], 1);
    test::expect_eq(star1_all[1], 2);
    std::println(std::cout, "  ✓ find_in_star matches star-level ships");

    // Test find_on_planet
    auto planet2_alive =
        repo.find_on_planet(starnum_t{1}, planetnum_t{2}, true);
    test::expect_eq(planet2_alive.size(), 1);
    test::expect_eq(planet2_alive[0], 3);

    auto planet2_all = repo.find_on_planet(starnum_t{1}, planetnum_t{2}, false);
    test::expect_eq(planet2_all.size(), 2);
    test::expect_eq(planet2_all[0], 3);
    test::expect_eq(planet2_all[1], 4);
    std::println(std::cout, "  ✓ find_on_planet matches planet-level ships");

    // Test find_in_hangar
    auto hangar_alive = repo.find_in_hangar(shipnum_t{1}, true);
    test::expect_eq(hangar_alive.size(), 1);
    test::expect_eq(hangar_alive[0], 5);
    std::println(std::cout, "  ✓ find_in_hangar matches carrier docked ships");

    // Test find_by_owner
    auto p1_alive = repo.find_by_owner(player_t{1}, true);
    test::expect_eq(p1_alive.size(), 2);
    test::expect_eq(p1_alive[0], 1);
    test::expect_eq(p1_alive[1], 5);

    auto p1_all = repo.find_by_owner(player_t{1}, false);
    test::expect_eq(p1_all.size(), 3);
    test::expect_eq(p1_all[0], 1);
    test::expect_eq(p1_all[1], 2);
    test::expect_eq(p1_all[2], 5);
    std::println(std::cout, "  ✓ find_by_owner matches player ships");

    // Test find_alive
    auto all_alive = repo.find_alive();
    test::expect_eq(all_alive.size(), 3);
    test::expect_eq(all_alive[0], 1);
    test::expect_eq(all_alive[1], 3);
    test::expect_eq(all_alive[2], 5);
    std::println(std::cout, "  ✓ find_alive matches all alive ships");
  }

  std::println(std::cout, "\nAll ShipRepository tests passed!");
  return 0;
}
