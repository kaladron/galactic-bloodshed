// SPDX-License-Identifier: Apache-2.0

/// \file sector_repository_test.cc
/// \brief Unit tests for SectorRepository CRUD operations and SectorMap SQLite
/// JSON persistence.

import dallib;
import gblib;
import test;
import std;

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
  test_planet.dimensions() = Coordinates{10, 10};

  // Create a test sector using NEW PATTERN
  sector_struct test_data{};
  test_data.coords = {5, 7};
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
  test_data.condition = SectorType::SEC_LAND;

  Sector test_sector(test_data);

  // Save sector
  std::println(std::cout, "Save sector...");
  bool saved = repo.save_sector(test_sector, test_planet.star_id(),
                                test_planet.planet_order(), 5, 7);
  test::expect_true(saved, "Failed to save sector");
  std::println(std::cout, "  ✓ Sector saved successfully");

  // Retrieve sector by location
  std::println(std::cout, "Retrieve sector by location...");
  auto retrieved =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 5, 7);
  test::expect_true(retrieved.has_value(), "Failed to retrieve sector");
  std::println(std::cout, "  ✓ Sector retrieved successfully");

  // Verify data integrity using accessor methods
  std::println(std::cout, "Verify data integrity...");
  test::expect_eq(retrieved->coords(), test_data.coords);
  test::expect_eq(retrieved->get_eff(), test_data.eff);
  test::expect_eq(retrieved->get_fert(), test_data.fert);
  test::expect_eq(retrieved->get_mobilization(), test_data.mobilization);
  test::expect_eq(retrieved->get_crystals(), test_data.crystals);
  test::expect_eq(retrieved->get_resource(), test_data.resource);
  test::expect_eq(retrieved->get_popn(), test_data.popn);
  test::expect_eq(retrieved->get_troops(), test_data.troops);
  test::expect_eq(retrieved->get_owner(), test_data.owner);
  test::expect_eq(retrieved->get_race(), test_data.race);
  test::expect_eq(retrieved->get_type(), test_data.type);
  test::expect_eq(retrieved->get_condition(), test_data.condition);
  std::println(std::cout, "  ✓ All fields match original");

  // Update sector using setters
  std::println(std::cout, "Update sector...");
  retrieved->set_efficiency_bounded(90);
  retrieved->set_popn_exact(15000);
  retrieved->set_crystals(150);
  saved = repo.save_sector(*retrieved, test_planet.star_id(),
                           test_planet.planet_order(), 5, 7);
  test::expect_true(saved, "Failed to update sector");
  std::println(std::cout, "  ✓ Sector updated successfully");

  // Retrieve updated sector
  std::println(std::cout, "Retrieve updated sector...");
  auto updated =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 5, 7);
  test::expect_true(updated.has_value(), "Failed to retrieve updated sector");
  test::expect_eq(updated->get_eff(), 90);
  test::expect_eq(updated->get_popn(), 15000);
  test::expect_eq(updated->get_crystals(), 150);
  std::println(std::cout, "  ✓ Updated values verified");

  // Save multiple sectors...
  std::println(std::cout, "Save multiple sectors...");
  sector_struct data2{};
  data2.coords = {3, 4};
  data2.eff = 60;
  data2.fert = 45;
  data2.owner = 1;
  data2.type = SectorType::SEC_SEA;
  Sector sector2(data2);
  repo.save_sector(sector2, test_planet.star_id(), test_planet.planet_order(),
                   3, 4);

  sector_struct data3{};
  data3.coords = {8, 2};
  data3.eff = 80;
  data3.fert = 30;
  data3.owner = 1;
  data3.type = SectorType::SEC_MOUNT;
  Sector sector3(data3);
  repo.save_sector(sector3, test_planet.star_id(), test_planet.planet_order(),
                   8, 2);
  std::println(std::cout, "  ✓ Multiple sectors saved");

  // Retrieve different sectors
  std::println(std::cout, "Retrieve different sectors...");
  auto sec2 =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 3, 4);
  test::expect_true(sec2.has_value());
  test::expect_eq(sec2->get_type(), SectorType::SEC_SEA);
  test::expect_eq(sec2->get_x(), 3);
  test::expect_eq(sec2->get_y(), 4);

  auto sec3 =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 8, 2);
  test::expect_true(sec3.has_value());
  test::expect_eq(sec3->get_type(), SectorType::SEC_MOUNT);
  test::expect_eq(sec3->get_x(), 8);
  test::expect_eq(sec3->get_y(), 2);
  std::println(std::cout, "  ✓ Different sectors retrieved correctly");

  // Find non-existent sector
  std::println(std::cout, "Find non-existent sector...");
  auto not_found = repo.find_sector(test_planet.star_id(),
                                    test_planet.planet_order(), 99, 99);
  test::expect_false(not_found.has_value(),
                     "Should not find non-existent sector");
  std::println(std::cout,
               "  ✓ Correctly returns nullopt for non-existent sector");

  // Sectors on different planets don't interfere
  std::println(std::cout, "Different planets...");
  Planet planet2{};
  planet2.star_id() = 5;
  planet2.planet_order() = 2;
  planet2.dimensions() = Coordinates{10, 10};

  sector_struct data_p2{};
  data_p2.coords = {5, 7};  // Same coordinates as sector on planet 1
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
  test::expect_true(p1_sec.has_value());
  test::expect_true(p2_sec.has_value());
  test::expect_eq(p1_sec->get_owner(), 1);
  test::expect_eq(p2_sec->get_owner(), 2);
  std::println(std::cout, "  ✓ Sectors on different planets handled correctly");

  // Save and load SectorMap (bulk operation)
  std::println(std::cout, "Save and load SectorMap (bulk)...");
  Planet small_planet{};
  small_planet.star_id() = 10;
  small_planet.planet_order() = 3;
  small_planet.dimensions() = Coordinates{3, 3};

  // Create a sector map with all sectors initialized
  SectorMap test_map(small_planet);  // true = initialize all sectors
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      auto& sec = test_map.get(Coordinates{x, y});
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
  test::expect_true(map_saved, "Failed to save sector map");
  std::println(std::cout, "  ✓ SectorMap saved successfully");

  // Load SectorMap
  std::println(std::cout, "Load SectorMap...");
  SectorMap loaded_map = repo.load_map(small_planet);

  // Verify all sectors loaded correctly
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      const auto& original = test_map.get(Coordinates{x, y});
      const auto& loaded = loaded_map.get(Coordinates{x, y});
      test::expect_eq(loaded.get_x(), original.get_x());
      test::expect_eq(loaded.get_y(), original.get_y());
      test::expect_eq(loaded.get_eff(), original.get_eff());
      test::expect_eq(loaded.get_fert(), original.get_fert());
      test::expect_eq(loaded.get_popn(), original.get_popn());
      test::expect_eq(loaded.get_owner(), original.get_owner());
      test::expect_eq(loaded.get_type(), original.get_type());
    }
  }
  std::println(std::cout, "  ✓ SectorMap loaded and verified");

  // Update and save SectorMap...
  std::println(std::cout, "Update and save SectorMap...");
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      auto& sec = loaded_map.get(Coordinates{x, y});
      sec.improve_efficiency(10);  // Increase efficiency by 10
      sec.add_popn(500);           // Add population
    }
  }

  map_saved = repo.save_map(loaded_map);
  test::expect_true(map_saved, "Failed to save updated map");

  // Reload and verify updates
  SectorMap updated_map = repo.load_map(small_planet);
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      const auto& original = test_map.get(Coordinates{x, y});
      const auto& updated = updated_map.get(Coordinates{x, y});
      test::expect_eq(updated.get_eff(), original.get_eff() + 10);
      test::expect_eq(updated.get_popn(), original.get_popn() + 500);
    }
  }
  std::println(std::cout, "  ✓ SectorMap updates saved and verified");

  // New load() method working with sector_struct directly
  std::println(std::cout, "New load() method (sector_struct)...");
  sector_struct loaded_struct =
      repo.load(test_planet.star_id(), test_planet.planet_order(), 5, 7);
  test::expect_eq(loaded_struct.coords.x, 5);
  test::expect_eq(loaded_struct.coords.y, 7);
  test::expect_eq(loaded_struct.eff, 90);        // From Test 5 update
  test::expect_eq(loaded_struct.popn, 15000);    // From Test 5 update
  test::expect_eq(loaded_struct.crystals, 150);  // From Test 5 update
  std::println(std::cout, "  ✓ load() returns sector_struct correctly");

  // New save() method working with sector_struct directly
  std::println(std::cout, "New save() method (sector_struct)...");
  sector_struct new_struct{};
  new_struct.coords = {9, 9};
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
  new_struct.condition = SectorType::SEC_LAND;

  repo.save(test_planet.star_id(), test_planet.planet_order(), 9, 9,
            new_struct);
  std::println(std::cout, "  ✓ save() with sector_struct completed");

  // Verify new save() persisted correctly using load()
  std::println(std::cout, "Verify save() persisted data...");
  sector_struct verified =
      repo.load(test_planet.star_id(), test_planet.planet_order(), 9, 9);
  test::expect_eq(verified.coords, new_struct.coords);
  test::expect_eq(verified.eff, new_struct.eff);
  test::expect_eq(verified.fert, new_struct.fert);
  test::expect_eq(verified.mobilization, new_struct.mobilization);
  test::expect_eq(verified.crystals, new_struct.crystals);
  test::expect_eq(verified.resource, new_struct.resource);
  test::expect_eq(verified.popn, new_struct.popn);
  test::expect_eq(verified.troops, new_struct.troops);
  test::expect_eq(verified.owner, new_struct.owner);
  test::expect_eq(verified.race, new_struct.race);
  test::expect_eq(verified.type, new_struct.type);
  test::expect_eq(verified.condition, new_struct.condition);
  std::println(std::cout, "  ✓ Data persisted and retrieved correctly");

  // Verify load() returns default sector_struct for non-existent
  std::println(std::cout, "load() with non-existent sector...");
  sector_struct empty =
      repo.load(test_planet.star_id(), test_planet.planet_order(), 99, 99);
  // Default-constructed sector_struct should have zero/default values
  test::expect_eq(empty.popn, 0);
  test::expect_eq(empty.owner, 0);
  std::println(std::cout,
               "  ✓ load() returns default sector_struct for non-existent");

  // Round-trip test with both new methods
  std::println(std::cout, "Round-trip test (save then load)...");
  sector_struct roundtrip{};
  roundtrip.coords = {1, 1};
  roundtrip.eff = 42;
  roundtrip.fert = 73;
  roundtrip.popn = 12345;
  roundtrip.owner = 3;
  roundtrip.type = SectorType::SEC_FOREST;

  repo.save(test_planet.star_id(), test_planet.planet_order(), 1, 1, roundtrip);
  sector_struct retrieved_rt =
      repo.load(test_planet.star_id(), test_planet.planet_order(), 1, 1);

  test::expect_eq(retrieved_rt.coords, roundtrip.coords);
  test::expect_eq(retrieved_rt.eff, roundtrip.eff);
  test::expect_eq(retrieved_rt.fert, roundtrip.fert);
  test::expect_eq(retrieved_rt.popn, roundtrip.popn);
  test::expect_eq(retrieved_rt.owner, roundtrip.owner);
  test::expect_eq(retrieved_rt.type, roundtrip.type);
  std::println(std::cout, "  ✓ Round-trip save/load works correctly");

  std::println(std::cout, "\nAll SectorRepository tests passed!");
  return 0;
}
