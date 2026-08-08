// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std;

#include <cassert>

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
  assert(saved && "Failed to save ship");
  std::println(std::cout, "  ✓ Ship saved successfully");

  // Retrieve by ship number
  std::println(std::cout, "Retrieve ship by number...");
  auto retrieved = repo.find_by_number(1);
  assert(retrieved.has_value() && "Failed to retrieve ship");
  std::println(std::cout, "  ✓ Ship retrieved successfully");

  // Verify data integrity
  std::println(std::cout, "Verify data integrity...");
  assert(retrieved->number() == test_ship.number());
  assert(retrieved->owner() == test_ship.owner());
  assert(retrieved->governor() == test_ship.governor());
  assert(retrieved->name() == test_ship.name());
  assert(retrieved->shipclass() == test_ship.shipclass());
  assert(retrieved->race() == test_ship.race());
  assert(retrieved->xpos() == test_ship.xpos());
  assert(retrieved->ypos() == test_ship.ypos());
  assert(retrieved->fuel() == test_ship.fuel());
  assert(retrieved->mass() == test_ship.mass());
  assert(retrieved->armor() == test_ship.armor());
  assert(retrieved->size() == test_ship.size());
  assert(retrieved->max_crew() == test_ship.max_crew());
  assert(retrieved->tech() == test_ship.tech());
  assert(retrieved->type() == test_ship.type());
  assert(retrieved->active() == test_ship.active());
  assert(retrieved->alive() == test_ship.alive());
  std::println(std::cout, "  ✓ All fields match original");

  // Update ship
  std::println(std::cout, "Update ship...");
  retrieved->fuel() = 3000.0;
  retrieved->damage() = 50;
  retrieved->xpos() = 150.0;
  saved = repo.save(*retrieved);
  assert(saved && "Failed to update ship");
  std::println(std::cout, "  ✓ Ship updated successfully");

  // Retrieve updated ship
  std::println(std::cout, "Retrieve updated ship...");
  auto updated = repo.find_by_number(1);
  assert(updated.has_value() && "Failed to retrieve updated ship");
  assert(updated->fuel() == 3000.0);
  assert(updated->damage() == 50);
  assert(updated->xpos() == 150.0);
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
  assert(count == 3 && "Should have 3 ships");
  std::println(std::cout, "  ✓ Ship count correct: {}", count);

  // Next available ship number (should find gap at 3)
  std::println(std::cout, "Next available ship number...");
  shipnum_t next_id = repo.next_ship_number();
  assert(next_id == 3 && "Should return 3 (first gap)");
  std::println(std::cout, "  ✓ Next ship number is: {}", next_id);

  // Delete a ship
  std::println(std::cout, "Delete ship...");
  repo.delete_ship(2);
  auto deleted = repo.find_by_number(2);
  assert(!deleted.has_value() && "Ship should be deleted");
  std::println(std::cout, "  ✓ Ship deleted successfully");

  // Count after deletion
  std::println(std::cout, "Count after deletion...");
  count = repo.count_all_ships();
  assert(count == 2 && "Should have 2 ships after deletion");
  std::println(std::cout, "  ✓ Ship count correct after deletion: {}", count);

  // Find non-existent ship
  std::println(std::cout, "Find non-existent ship...");
  auto not_found = repo.find_by_number(999);
  assert(!not_found.has_value() && "Should not find non-existent ship");
  std::println(std::cout,
               "  ✓ Correctly returns nullopt for non-existent ship");

  // List ships
  std::println(std::cout, "List ships...");
  auto ship_ids = repo.list_ids();
  assert(ship_ids.size() == 2);
  assert(ship_ids[0] == 1 && ship_ids[1] == 5);
  std::println(std::cout, "  ✓ list_ids returns active ship IDs in order");

  std::println(std::cout, "\nAll ShipRepository tests passed!");
  return 0;
}
