// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <sqlite3.h>

#include <cassert>

int main() {
  // CRITICAL: Always create in-memory database BEFORE calling initsqldata()
  Sql db(":memory:");

  // Initialize database tables - this creates all required tables
  initsqldata();

  // Create a test planet to associate sectors with
  Planet test_planet{};
  test_planet.planet_id = 1;
  test_planet.Maxx = 50;
  test_planet.Maxy = 50;

  // Create a test sector with various values
  Sector test_sector{};
  test_sector.x = 10;
  test_sector.y = 20;
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

  // Test putsector - stores in SQLite as JSON
  putsector(test_sector, test_planet);

  // Test getsector - reads from SQLite
  Sector retrieved_sector = getsector(test_planet, 10, 20);

  // Verify all fields
  assert(retrieved_sector.x == test_sector.x);
  assert(retrieved_sector.y == test_sector.y);
  assert(retrieved_sector.eff == test_sector.eff);
  assert(retrieved_sector.fert == test_sector.fert);
  assert(retrieved_sector.mobilization == test_sector.mobilization);
  assert(retrieved_sector.crystals == test_sector.crystals);
  assert(retrieved_sector.resource == test_sector.resource);
  assert(retrieved_sector.popn == test_sector.popn);
  assert(retrieved_sector.troops == test_sector.troops);
  assert(retrieved_sector.owner == test_sector.owner);
  assert(retrieved_sector.race == test_sector.race);
  assert(retrieved_sector.type == test_sector.type);
  assert(retrieved_sector.condition == test_sector.condition);

  // Test updating the sector
  test_sector.eff = 90;
  test_sector.popn = 15000;
  putsector(test_sector, test_planet);

  // Retrieve again and verify changes
  Sector updated_sector = getsector(test_planet, 10, 20);
  assert(updated_sector.eff == 90);
  assert(updated_sector.popn == 15000);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println("Sector SQLite storage test passed!");
  return 0;
}
