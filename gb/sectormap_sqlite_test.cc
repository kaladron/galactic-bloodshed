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

  // Populate the map with different sector types and values using NEW PATTERN
  for (int y = 0; y < test_planet.Maxy(); y++) {
    for (int x = 0; x < test_planet.Maxx(); x++) {
      auto& sector = test_smap.get(x, y);
      sector.set_x(x);
      sector.set_y(y);
      sector.set_eff(50 + (x * y));
      sector.set_fert(30 + x);
      sector.set_mobilization(10 + y);
      sector.set_crystals(x * 10);
      sector.set_resource(y * 20);
      sector.set_popn((x + 1) * (y + 1) * 100);
      sector.set_troops(x + y);
      sector.set_owner((x + y) % 2 + 1);  // Alternate between player 1 and 2
      sector.set_race(sector.get_owner());

      // Vary sector types
      if (x == 0 || x == test_planet.Maxx() - 1 || y == 0 ||
          y == test_planet.Maxy() - 1) {
        sector.set_type(SectorType::SEC_SEA);
        sector.set_condition(SectorType::SEC_SEA);
      } else if ((x + y) % 3 == 0) {
        sector.set_type(SectorType::SEC_MOUNT);
        sector.set_condition(SectorType::SEC_MOUNT);
      } else if ((x + y) % 3 == 1) {
        sector.set_type(SectorType::SEC_LAND);
        sector.set_condition(SectorType::SEC_LAND);
      } else {
        sector.set_type(SectorType::SEC_FOREST);
        sector.set_condition(SectorType::SEC_FOREST);
      }
    }
  }

  // Test putsmap - stores entire map in SQLite as JSON
  putsmap(test_smap, test_planet);

  // Test getsmap - reads entire map from SQLite
  SectorMap retrieved_smap = getsmap(test_planet);

  // Verify all sectors were stored and retrieved correctly using accessor
  // methods
  for (int y = 0; y < test_planet.Maxy(); y++) {
    for (int x = 0; x < test_planet.Maxx(); x++) {
      const auto& original = test_smap.get(x, y);
      const auto& retrieved = retrieved_smap.get(x, y);

      assert(retrieved.get_x() == original.get_x());
      assert(retrieved.get_y() == original.get_y());
      assert(retrieved.get_eff() == original.get_eff());
      assert(retrieved.get_fert() == original.get_fert());
      assert(retrieved.get_mobilization() == original.get_mobilization());
      assert(retrieved.get_crystals() == original.get_crystals());
      assert(retrieved.get_resource() == original.get_resource());
      assert(retrieved.get_popn() == original.get_popn());
      assert(retrieved.get_troops() == original.get_troops());
      assert(retrieved.get_owner() == original.get_owner());
      assert(retrieved.get_race() == original.get_race());
      assert(retrieved.get_type() == original.get_type());
      assert(retrieved.get_condition() == original.get_condition());
    }
  }

  // Test updating some sectors in the map
  for (int i = 0; i < 5; i++) {
    auto& sector = test_smap.get(i, i);
    sector.set_eff(100);
    sector.set_popn(50000);
    sector.set_crystals(999);
  }

  // Save the updated map
  putsmap(test_smap, test_planet);

  // Retrieve again and verify the updates
  SectorMap updated_smap = getsmap(test_planet);

  for (int i = 0; i < 5; i++) {
    const auto& updated = updated_smap.get(i, i);
    assert(updated.get_eff() == 100);
    assert(updated.get_popn() == 50000);
    assert(updated.get_crystals() == 999);
  }

  // Verify other sectors remain unchanged
  const auto& unchanged = updated_smap.get(5, 5);
  assert(unchanged.get_eff() == test_smap.get(5, 5).get_eff());
  assert(unchanged.get_popn() == test_smap.get(5, 5).get_popn());

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
