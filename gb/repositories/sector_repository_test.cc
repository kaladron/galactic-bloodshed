// SPDX-License-Identifier: Apache-2.0

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

  // Create a test sector
  Sector test_sector{};
  test_sector.x = 5;
  test_sector.y = 7;
  test_sector.eff = 75;
  test_sector.fert = 50;
  test_sector.mobilization = 25;
  test_sector.crystals = 100;
  test_sector.resource = 500;
  test_sector.popn = 10000;
  test_sector.troops = 250;
  test_sector.owner = 1;
  test_sector.race = 1;
  test_sector.type = SectorType::SEC_LAND;
  test_sector.condition = 0;

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

  // Test 3: Verify data integrity
  std::println("Test 3: Verify data integrity...");
  assert(retrieved->x == test_sector.x);
  assert(retrieved->y == test_sector.y);
  assert(retrieved->eff == test_sector.eff);
  assert(retrieved->fert == test_sector.fert);
  assert(retrieved->mobilization == test_sector.mobilization);
  assert(retrieved->crystals == test_sector.crystals);
  assert(retrieved->resource == test_sector.resource);
  assert(retrieved->popn == test_sector.popn);
  assert(retrieved->troops == test_sector.troops);
  assert(retrieved->owner == test_sector.owner);
  assert(retrieved->race == test_sector.race);
  assert(retrieved->type == test_sector.type);
  assert(retrieved->condition == test_sector.condition);
  std::println("  ✓ All fields match original");

  // Test 4: Update sector
  std::println("Test 4: Update sector...");
  retrieved->eff = 90;
  retrieved->popn = 15000;
  retrieved->crystals = 150;
  saved = repo.save_sector(*retrieved, test_planet.star_id(),
                           test_planet.planet_order(), 5, 7);
  assert(saved && "Failed to update sector");
  std::println("  ✓ Sector updated successfully");

  // Test 5: Retrieve updated sector
  std::println("Test 5: Retrieve updated sector...");
  auto updated =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 5, 7);
  assert(updated.has_value() && "Failed to retrieve updated sector");
  assert(updated->eff == 90);
  assert(updated->popn == 15000);
  assert(updated->crystals == 150);
  std::println("  ✓ Updated values verified");

  // Test 6: Save multiple sectors on same planet
  std::println("Test 6: Save multiple sectors...");
  Sector sector2{};
  sector2.x = 3;
  sector2.y = 4;
  sector2.eff = 60;
  sector2.fert = 45;
  sector2.owner = 1;
  sector2.type = SectorType::SEC_SEA;
  repo.save_sector(sector2, test_planet.star_id(), test_planet.planet_order(),
                   3, 4);

  Sector sector3{};
  sector3.x = 8;
  sector3.y = 2;
  sector3.eff = 80;
  sector3.fert = 30;
  sector3.owner = 1;
  sector3.type = SectorType::SEC_MOUNT;
  repo.save_sector(sector3, test_planet.star_id(), test_planet.planet_order(),
                   8, 2);
  std::println("  ✓ Multiple sectors saved");

  // Test 7: Retrieve different sectors
  std::println("Test 7: Retrieve different sectors...");
  auto sec2 =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 3, 4);
  assert(sec2.has_value());
  assert(sec2->type == SectorType::SEC_SEA);
  assert(sec2->x == 3 && sec2->y == 4);

  auto sec3 =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 8, 2);
  assert(sec3.has_value());
  assert(sec3->type == SectorType::SEC_MOUNT);
  assert(sec3->x == 8 && sec3->y == 2);
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

  Sector sector_p2{};
  sector_p2.x = 5;
  sector_p2.y = 7;  // Same coordinates as sector on planet 1
  sector_p2.eff = 70;
  sector_p2.fert = 50;
  sector_p2.owner = 2;  // Different owner
  sector_p2.type = SectorType::SEC_LAND;
  repo.save_sector(sector_p2, planet2.star_id(), planet2.planet_order(), 5, 7);

  // Both sectors should exist independently
  auto p1_sec =
      repo.find_sector(test_planet.star_id(), test_planet.planet_order(), 5, 7);
  auto p2_sec =
      repo.find_sector(planet2.star_id(), planet2.planet_order(), 5, 7);
  assert(p1_sec.has_value() && p2_sec.has_value());
  assert(p1_sec->owner == 1);
  assert(p2_sec->owner == 2);
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
      sec.x = x;
      sec.y = y;
      sec.eff = 50 + x + y;
      sec.fert = 40;
      sec.popn = 1000 + (x + y);  // Simple population value
      sec.owner = 1;
      sec.type = (x + y) % 2 == 0 ? SectorType::SEC_LAND : SectorType::SEC_SEA;
    }
  }

  // Save entire map
  bool map_saved = repo.save_map(test_map, small_planet);
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
      assert(loaded.x == original.x);
      assert(loaded.y == original.y);
      assert(loaded.eff == original.eff);
      assert(loaded.fert == original.fert);
      assert(loaded.popn == original.popn);
      assert(loaded.owner == original.owner);
      assert(loaded.type == original.type);
    }
  }
  std::println("  ✓ SectorMap loaded and verified");

  // Test 12: Update SectorMap and save
  std::println("Test 12: Update and save SectorMap...");
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      auto& sec = loaded_map.get(x, y);
      sec.eff += 10;    // Increase efficiency by 10
      sec.popn += 500;  // Add population
    }
  }

  map_saved = repo.save_map(loaded_map, small_planet);
  assert(map_saved && "Failed to save updated map");

  // Reload and verify updates
  SectorMap updated_map = repo.load_map(small_planet);
  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      const auto& original = test_map.get(x, y);
      const auto& updated = updated_map.get(x, y);
      assert(updated.eff == original.eff + 10);
      assert(updated.popn == original.popn + 500);
    }
  }
  std::println("  ✓ SectorMap updates saved and verified");

  std::println("\nAll SectorRepository tests passed!");
  return 0;
}
