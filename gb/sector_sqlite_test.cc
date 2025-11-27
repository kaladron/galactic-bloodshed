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
  Planet test_planet{};
  test_planet.star_id() = 5;
  test_planet.planet_order() = 1;
  test_planet.Maxx() = 50;
  test_planet.Maxy() = 50;

  // Create a test sector with various values using NEW PATTERN
  sector_struct test_data{};
  test_data.x = 10;
  test_data.y = 20;
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

  // Test putsector - stores in SQLite as JSON
  putsector(test_sector, test_planet);

  // Test getsector - reads from SQLite
  Sector retrieved_sector = getsector(test_planet, 10, 20);

  // Verify all fields using NEW PATTERN (accessor methods)
  assert(retrieved_sector.get_x() == test_data.x);
  assert(retrieved_sector.get_y() == test_data.y);
  assert(retrieved_sector.get_eff() == test_data.eff);
  assert(retrieved_sector.get_fert() == test_data.fert);
  assert(retrieved_sector.get_mobilization() == test_data.mobilization);
  assert(retrieved_sector.get_crystals() == test_data.crystals);
  assert(retrieved_sector.get_resource() == test_data.resource);
  assert(retrieved_sector.get_popn() == test_data.popn);
  assert(retrieved_sector.get_troops() == test_data.troops);
  assert(retrieved_sector.get_owner() == test_data.owner);
  assert(retrieved_sector.get_race() == test_data.race);
  assert(retrieved_sector.get_type() == test_data.type);
  assert(retrieved_sector.get_condition() == test_data.condition);

  // Test updating the sector using NEW PATTERN (setters)
  test_sector.set_eff(90);
  test_sector.set_popn(15000);
  putsector(test_sector, test_planet);

  // Retrieve again and verify changes using accessor methods
  Sector updated_sector = getsector(test_planet, 10, 20);
  assert(updated_sector.get_eff() == 90);
  assert(updated_sector.get_popn() == 15000);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println("Sector SQLite storage test passed!");
  return 0;
}
