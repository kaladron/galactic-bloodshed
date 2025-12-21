// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <sqlite3.h>

#include <cassert>

int main() {
  // Initialize database using Database class (in-memory for testing)
  Database db(":memory:");

  // Initialize database tables - this will create the tbl_power table
  initialize_schema(db);

  power test_power[MAXPLAYERS];

  // Initialize some test data for a few players
  test_power[0].id = 1;  // CRITICAL: Set power id
  test_power[0].troops = 1000;
  test_power[0].popn = 50000;
  test_power[0].resource = 25000;
  test_power[0].fuel = 10000;
  test_power[0].destruct = 500;
  test_power[0].ships_owned = 20;
  test_power[0].planets_owned = 3;
  test_power[0].sectors_owned = 150;
  test_power[0].money = 100000;
  test_power[0].sum_mob = 75;
  test_power[0].sum_eff = 85;

  test_power[1].id = 2;  // CRITICAL: Set power id
  test_power[1].troops = 800;
  test_power[1].popn = 40000;
  test_power[1].resource = 20000;
  test_power[1].fuel = 8000;
  test_power[1].destruct = 400;
  test_power[1].ships_owned = 15;
  test_power[1].planets_owned = 2;
  test_power[1].sectors_owned = 120;
  test_power[1].money = 80000;
  test_power[1].sum_mob = 60;
  test_power[1].sum_eff = 70;

  // Initialize remaining power entries to zero (but with id set!)
  for (int i = 2; i < MAXPLAYERS; i++) {
    test_power[i] = power{};
    test_power[i].id = i + 1;  // CRITICAL: Set id for power records
  }

  // Test EntityManager - stores and retrieves power data
  // First save using repository
  JsonStore store(db);
  PowerRepository power_repo(store);
  for (int i = 0; i < MAXPLAYERS; i++) {
    power_repo.save(test_power[i]);
  }

  // Now use EntityManager to retrieve
  EntityManager em(db);
  power loaded_power[MAXPLAYERS];
  for (int i = 0; i < MAXPLAYERS; i++) {
    loaded_power[i] = power{};
  }

  // Retrieve from EntityManager
  for (int i = 0; i < MAXPLAYERS; i++) {
    const auto* power_ptr = em.peek_power(i + 1);
    assert(power_ptr);  // Should exist now
    loaded_power[i] = *power_ptr;
  }

  // Verify the data matches
  assert(loaded_power[0].troops == test_power[0].troops);
  assert(loaded_power[0].popn == test_power[0].popn);
  assert(loaded_power[0].resource == test_power[0].resource);
  assert(loaded_power[0].fuel == test_power[0].fuel);
  assert(loaded_power[0].destruct == test_power[0].destruct);
  assert(loaded_power[0].ships_owned == test_power[0].ships_owned);
  assert(loaded_power[0].planets_owned == test_power[0].planets_owned);
  assert(loaded_power[0].sectors_owned == test_power[0].sectors_owned);
  assert(loaded_power[0].money == test_power[0].money);
  assert(loaded_power[0].sum_mob == test_power[0].sum_mob);
  assert(loaded_power[0].sum_eff == test_power[0].sum_eff);

  assert(loaded_power[1].troops == test_power[1].troops);
  assert(loaded_power[1].popn == test_power[1].popn);
  assert(loaded_power[1].resource == test_power[1].resource);
  assert(loaded_power[1].fuel == test_power[1].fuel);
  assert(loaded_power[1].destruct == test_power[1].destruct);
  assert(loaded_power[1].ships_owned == test_power[1].ships_owned);
  assert(loaded_power[1].planets_owned == test_power[1].planets_owned);
  assert(loaded_power[1].sectors_owned == test_power[1].sectors_owned);
  assert(loaded_power[1].money == test_power[1].money);
  assert(loaded_power[1].sum_mob == test_power[1].sum_mob);
  assert(loaded_power[1].sum_eff == test_power[1].sum_eff);

  std::println("All power JSON serialization tests passed!");
  return 0;
}
