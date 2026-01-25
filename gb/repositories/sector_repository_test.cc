// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import dallib;
import std.compat;

#include <cassert>

int main() {
  // Create in-memory database and initialize schema
  Database db(":memory:");
  initialize_schema(db);

  // Create JsonStore and SectorRepository
  JsonStore store(db);
  SectorRepository repo(store);

  // Create a test planet to associate sectors with
  Planet test_planet{};
  test_planet.star_id() = 5;
  test_planet.planet_order() = 1;
  test_planet.Maxx() = 10;
  test_planet.Maxy() = 10;

  // Create a test sector using NEW PATTERN
  sector_struct test_data{};
  test_data.x = 5;
  test_data.y = 7;
  test_data.eff = 75;
  test_data.fert = 50;
  test_data.mobilization = 25;
  test_data.crystals = 100;
  test_data.resource = 500;
  test_data.popn = 10000;
  test_data.troops = 250;
  test_data.owner = 1;
  test_data.race = 1;
  test_data.type = SectorType::SEC_LAND;
  test_data.condition = 0;

  Sector test_sector(test_data);

  // Test 1: Save sector
  std::println("Test 1: Save sector...");
  bool saved = repo.save_sector(test_sector, test_planet.star_id(),
                                test_planet.planet_order(), 5, 7);
  assert(saved && "Failed to save sector");
  std::println("  ✓ Sector saved successfully");

  // Test 2: Retrieve sector by location
  std::println("Test 2: Retrieve sector by location...");
  auto retrieved =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 5, 7);
  assert(retrieved.has_value() && "Failed to retrieve sector");
  std::println("  ✓ Sector retrieved successfully");

  // Test 3: Verify data integrity using accessor methods
  std::println("Test 3: Verify data integrity...");
  assert(retrieved->get_x() == test_data.x);
  assert(retrieved->get_y() == test_data.y);
  assert(retrieved->get_eff() == test_data.eff);
  assert(retrieved->get_fert() == test_data.fert);
  assert(retrieved->get_mobilization() == test_data.mobilization);
  assert(retrieved->get_crystals() == test_data.crystals);
  assert(retrieved->get_resource() == test_data.resource);
  assert(retrieved->get_popn() == test_data.popn);
  assert(retrieved->get_troops() == test_data.troops);
  assert(retrieved->get_owner() == test_data.owner);
  assert(retrieved->get_race() == test_data.race);
  assert(retrieved->get_type() == test_data.type);
  assert(retrieved->get_condition() == test_data.condition);
  std::println("  ✓ All fields match original");

  // Test 4: Update sector using setters
  std::println("Test 4: Update sector...");
  retrieved->set_efficiency_bounded(90);
  retrieved->set_popn_exact(15000);
  retrieved->set_crystals(150);
  saved = repo.save_sector(*retrieved, test_planet.star_id(),
                           test_planet.planet_order(), 5, 7);
  assert(saved && "Failed to update sector");
  std::println("  ✓ Sector updated successfully");

  // Test 5: Retrieve updated sector
  std::println("Test 5: Retrieve updated sector...");
  auto updated =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 5, 7);
  assert(updated.has_value() && "Failed to retrieve updated sector");
  assert(updated->get_eff() == 90);
  assert(updated->get_popn() == 15000);
  assert(updated->get_crystals() == 150);
  std::println("  ✓ Updated values verified");

  // Test 6: Save multiple sectors...
  std::println("Test 6: Save multiple sectors...");
  sector_struct data2{};
  data2.x = 3;
  data2.y = 4;
  data2.eff = 60;
  data2.fert = 45;
  data2.owner = 1;
  data2.type = SectorType::SEC_SEA;
  Sector sector2(data2);
  repo.save_sector(sector2, test_planet.star_id(), test_planet.planet_order(),
                   3, 4);

  sector_struct data3{};
  data3.x = 8;
  data3.y = 2;
  data3.eff = 80;
  data3.fert = 30;
  data3.owner = 1;
  data3.type = SectorType::SEC_MOUNT;
  Sector sector3(data3);
  repo.save_sector(sector3, test_planet.star_id(), test_planet.planet_order(),
                   8, 2);
  std::println("  ✓ Multiple sectors saved");

  // Test 7: Retrieve different sectors
  std::println("Test 7: Retrieve different sectors...");
  auto sec2 =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 3, 4);
  assert(sec2.has_value());
  assert(sec2->get_type() == SectorType::SEC_SEA);
  assert(sec2->get_x() == 3 && sec2->get_y() == 4);

  auto sec3 =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 8, 2);
  assert(sec3.has_value());
  assert(sec3->get_type() == SectorType::SEC_MOUNT);
  assert(sec3->get_x() == 8 && sec3->get_y() == 2);
  std::println("  ✓ Different sectors retrieved correctly");

  // Test 8: Find non-existent sector
  std::println("Test 8: Find non-existent sector...");
  auto not_found = repo.find_sector(test_planet.star_id(),
                                    test_planet.planet_order(), 99, 99);
  assert(!not_found.has_value() && "Should not find non-existent sector");
  std::println("  ✓ Correctly returns nullopt for non-existent sector");

  // Test 9: Sectors on different planets don't interfere
  std::println("Test 9: Different planets...");
  Planet planet2{};
  planet2.star_id() = 5;
  planet2.planet_order() = 2;
  planet2.Maxx() = 10;
  planet2.Maxy() = 10;

  sector_struct data_p2{};
  data_p2.x = 5;
  data_p2.y = 7;  // Same coordinates as sector on planet 1
  data_p2.eff = 70;
  data_p2.fert = 50;
  data_p2.owner = 2;  // Different owner
  data_p2.type = SectorType::SEC_LAND;
  Sector sector_p2(data_p2);
  repo.save_sector(sector_p2, planet2.star_id(), planet2.planet_order(), 5, 7);

  // Both sectors should exist independently
  auto p1_sec =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 5, 7);
  auto p2_sec =
      repo.find_sector(planet2.star_id(), planet2.planet_order(), 5, 7);
  assert(p1_sec.has_value() && p2_sec.has_value());
  assert(p1_sec->get_owner() == 1);
  assert(p2_sec->get_owner() == 2);
  std::println("  ✓ Sectors on different planets handled correctly");

  // Test 10: Save and load SectorMap (bulk operation)
  std::println("Test 10: Save and load SectorMap (bulk)...");
  Planet small_planet{};
  small_planet.star_id() = 10;
  small_planet.planet_order() = 3;
  small_planet.Maxx() = 3;
  small_planet.Maxy() = 3;

  // Create a sector map with all sectors initialized
  SectorMap test_map(small_planet, true);  // true = initialize all sectors
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      auto& sec = test_map.get(x, y);
      sec.set_x(x);
      sec.set_y(y);
      sec.set_efficiency_bounded(50 + x + y);
      sec.set_fert(40);
      sec.set_popn_exact(1000 + (x + y));  // Simple population value
      sec.set_owner(1);
      sec.set_type((x + y) % 2 == 0 ? SectorType::SEC_LAND
                                    : SectorType::SEC_SEA);
    }
  }

  // Save entire map
  bool map_saved = repo.save_map(test_map);
  assert(map_saved && "Failed to save sector map");
  std::println("  ✓ SectorMap saved successfully");

  // Test 11: Load SectorMap
  std::println("Test 11: Load SectorMap...");
  SectorMap loaded_map = repo.load_map(small_planet);

  // Verify all sectors loaded correctly
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      const auto& original = test_map.get(x, y);
      const auto& loaded = loaded_map.get(x, y);
      assert(loaded.get_x() == original.get_x());
      assert(loaded.get_y() == original.get_y());
      assert(loaded.get_eff() == original.get_eff());
      assert(loaded.get_fert() == original.get_fert());
      assert(loaded.get_popn() == original.get_popn());
      assert(loaded.get_owner() == original.get_owner());
      assert(loaded.get_type() == original.get_type());
    }
  }
  std::println("  ✓ SectorMap loaded and verified");

  // Test 12: Update and save SectorMap...
  std::println("Test 12: Update and save SectorMap...");
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      auto& sec = loaded_map.get(x, y);
      sec.improve_efficiency(10);  // Increase efficiency by 10
      sec.add_popn(500);           // Add population
    }
  }

  map_saved = repo.save_map(loaded_map);
  assert(map_saved && "Failed to save updated map");

  // Reload and verify updates
  SectorMap updated_map = repo.load_map(small_planet);
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      const auto& original = test_map.get(x, y);
      const auto& updated = updated_map.get(x, y);
      assert(updated.get_eff() == original.get_eff() + 10);
      assert(updated.get_popn() == original.get_popn() + 500);
    }
  }
  std::println("  ✓ SectorMap updates saved and verified");

  // Test 13: New load() method working with sector_struct directly
  std::println("Test 13: New load() method (sector_struct)...");
  sector_struct loaded_struct =
      repo.load(test_planet.star_id(), test_planet.planet_order(), 5, 7);
  assert(loaded_struct.x == 5);
  assert(loaded_struct.y == 7);
  assert(loaded_struct.eff == 90);        // From Test 5 update
  assert(loaded_struct.popn == 15000);    // From Test 5 update
  assert(loaded_struct.crystals == 150);  // From Test 5 update
  std::println("  ✓ load() returns sector_struct correctly");

  // Test 14: New save() method working with sector_struct directly
  std::println("Test 14: New save() method (sector_struct)...");
  sector_struct new_struct{};
  new_struct.x = 9;
  new_struct.y = 9;
  new_struct.eff = 95;
  new_struct.fert = 85;
  new_struct.mobilization = 30;
  new_struct.crystals = 200;
  new_struct.resource = 750;
  new_struct.popn = 20000;
  new_struct.troops = 500;
  new_struct.owner = 1;
  new_struct.race = 1;
  new_struct.type = SectorType::SEC_LAND;
  new_struct.condition = 0;

  repo.save(test_planet.star_id(), test_planet.planet_order(), 9, 9,
            new_struct);
  std::println("  ✓ save() with sector_struct completed");

  // Test 15: Verify new save() persisted correctly using load()
  std::println("Test 15: Verify save() persisted data...");
  sector_struct verified =
      repo.load(test_planet.star_id(), test_planet.planet_order(), 9, 9);
  assert(verified.x == new_struct.x);
  assert(verified.y == new_struct.y);
  assert(verified.eff == new_struct.eff);
  assert(verified.fert == new_struct.fert);
  assert(verified.mobilization == new_struct.mobilization);
  assert(verified.crystals == new_struct.crystals);
  assert(verified.resource == new_struct.resource);
  assert(verified.popn == new_struct.popn);
  assert(verified.troops == new_struct.troops);
  assert(verified.owner == new_struct.owner);
  assert(verified.race == new_struct.race);
  assert(verified.type == new_struct.type);
  assert(verified.condition == new_struct.condition);
  std::println("  ✓ Data persisted and retrieved correctly");

  // Test 16: Verify load() returns default sector_struct for non-existent
  std::println("Test 16: load() with non-existent sector...");
  sector_struct empty =
      repo.load(test_planet.star_id(), test_planet.planet_order(), 99, 99);
  // Default-constructed sector_struct should have zero/default values
  assert(empty.popn == 0);
  assert(empty.owner == 0);
  std::println("  ✓ load() returns default sector_struct for non-existent");

  // Test 17: Round-trip test with both new methods
  std::println("Test 17: Round-trip test (save then load)...");
  sector_struct roundtrip{};
  roundtrip.x = 1;
  roundtrip.y = 1;
  roundtrip.eff = 42;
  roundtrip.fert = 73;
  roundtrip.popn = 12345;
  roundtrip.owner = 3;
  roundtrip.type = SectorType::SEC_FOREST;

  repo.save(test_planet.star_id(), test_planet.planet_order(), 1, 1, roundtrip);
  sector_struct retrieved_rt =
      repo.load(test_planet.star_id(), test_planet.planet_order(), 1, 1);

  assert(retrieved_rt.x == roundtrip.x);
  assert(retrieved_rt.y == roundtrip.y);
  assert(retrieved_rt.eff == roundtrip.eff);
  assert(retrieved_rt.fert == roundtrip.fert);
  assert(retrieved_rt.popn == roundtrip.popn);
  assert(retrieved_rt.owner == roundtrip.owner);
  assert(retrieved_rt.type == roundtrip.type);
  std::println("  ✓ Round-trip save/load works correctly");

  std::println("\nAll SectorRepository tests passed!");
  return 0;
}
