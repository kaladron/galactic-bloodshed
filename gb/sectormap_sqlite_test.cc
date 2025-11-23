// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <sqlite3.h>

#include <cassert>

int main() {
  // CRITICAL: Always create in-memory database BEFORE calling
  // initialize_schema()
  Database db(":memory:");

  // Initialize database tables - this creates all required tables
  initialize_schema(db);

  // Create a test planet to associate sectors with
  Planet test_planet{PlanetType::EARTH};
  test_planet.star_id() = 3;
  test_planet.planet_order() = 2;
  test_planet.Maxx() = 10;
  test_planet.Maxy() = 10;

  // Create a SectorMap with various sector types
  SectorMap test_smap(test_planet, true);

  // Populate the map with different sector types and values
  for (int y = 0; y < test_planet.Maxy(); y++) {
    for (int x = 0; x < test_planet.Maxx(); x++) {
      auto& sector = test_smap.get(x, y);
      sector.x = x;
      sector.y = y;
      sector.eff = 50 + (x * y);
      sector.fert = 30 + x;
      sector.mobilization = 10 + y;
      sector.crystals = x * 10;
      sector.resource = y * 20;
      sector.popn = (x + 1) * (y + 1) * 100;
      sector.troops = x + y;
      sector.owner = (x + y) % 2 + 1;  // Alternate between player 1 and 2
      sector.race = sector.owner;

      // Vary sector types
      if (x == 0 || x == test_planet.Maxx() - 1 || y == 0 ||
          y == test_planet.Maxy() - 1) {
        sector.type = SectorType::SEC_SEA;
        sector.condition = SectorType::SEC_SEA;
      } else if ((x + y) % 3 == 0) {
        sector.type = SectorType::SEC_MOUNT;
        sector.condition = SectorType::SEC_MOUNT;
      } else if ((x + y) % 3 == 1) {
        sector.type = SectorType::SEC_LAND;
        sector.condition = SectorType::SEC_LAND;
      } else {
        sector.type = SectorType::SEC_FOREST;
        sector.condition = SectorType::SEC_FOREST;
      }
    }
  }

  // Test putsmap - stores entire map in SQLite as JSON
  putsmap(test_smap, test_planet);

  // Test getsmap - reads entire map from SQLite
  SectorMap retrieved_smap = getsmap(test_planet);

  // Verify all sectors were stored and retrieved correctly
  for (int y = 0; y < test_planet.Maxy(); y++) {
    for (int x = 0; x < test_planet.Maxx(); x++) {
      const auto& original = test_smap.get(x, y);
      const auto& retrieved = retrieved_smap.get(x, y);

      assert(retrieved.x == original.x);
      assert(retrieved.y == original.y);
      assert(retrieved.eff == original.eff);
      assert(retrieved.fert == original.fert);
      assert(retrieved.mobilization == original.mobilization);
      assert(retrieved.crystals == original.crystals);
      assert(retrieved.resource == original.resource);
      assert(retrieved.popn == original.popn);
      assert(retrieved.troops == original.troops);
      assert(retrieved.owner == original.owner);
      assert(retrieved.race == original.race);
      assert(retrieved.type == original.type);
      assert(retrieved.condition == original.condition);
    }
  }

  // Test updating some sectors in the map
  for (int i = 0; i < 5; i++) {
    auto& sector = test_smap.get(i, i);
    sector.eff = 100;
    sector.popn = 50000;
    sector.crystals = 999;
  }

  // Save the updated map
  putsmap(test_smap, test_planet);

  // Retrieve again and verify the updates
  SectorMap updated_smap = getsmap(test_planet);

  for (int i = 0; i < 5; i++) {
    const auto& updated = updated_smap.get(i, i);
    assert(updated.eff == 100);
    assert(updated.popn == 50000);
    assert(updated.crystals == 999);
  }

  // Verify other sectors remain unchanged
  const auto& unchanged = updated_smap.get(5, 5);
  assert(unchanged.eff == test_smap.get(5, 5).eff);
  assert(unchanged.popn == test_smap.get(5, 5).popn);

  // Test with an empty planet (no sectors saved yet)
  // This should return a SectorMap with no sectors (empty vector)
  // Accessing it would throw, so we just verify it doesn't crash on creation
  Planet empty_planet{PlanetType::MARS};
  empty_planet.star_id() = 10;
  empty_planet.planet_order() = 0;
  empty_planet.Maxx() = 5;
  empty_planet.Maxy() = 5;

  SectorMap empty_smap = getsmap(empty_planet);
  // The map was created successfully but has no sectors in it
  // (this is the expected behavior for a planet with no saved sectors)

  // Database connection will be cleaned up automatically by Sql destructor

  std::println("SectorMap SQLite storage test passed!");
  return 0;
}
