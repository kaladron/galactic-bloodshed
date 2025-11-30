// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  // Initialize database using Database class (in-memory for testing)
  Database db(":memory:");

  // Initialize database tables - this will create the ship tables
  initialize_schema(db);

  // Test initial state - should have 0 ships
  shipnum_t initial_count = Numships();

  // Create test ships
  Ship test_ship1{};
  test_ship1.number() = 1;
  test_ship1.owner() = 1;
  test_ship1.governor() = 0;
  test_ship1.name() = "TestShip1";
  test_ship1.type() = ShipType::STYPE_SHUTTLE;
  test_ship1.alive() = 1;
  test_ship1.active() = 1;

  Ship test_ship2{};
  test_ship2.number() = 2;
  test_ship2.owner() = 1;
  test_ship2.governor() = 0;
  test_ship2.name() = "TestShip2";
  test_ship2.type() = ShipType::STYPE_CARGO;
  test_ship2.alive() = 1;
  test_ship2.active() = 1;

  Ship test_ship3{};
  test_ship3.number() = 3;
  test_ship3.owner() = 2;
  test_ship3.governor() = 0;
  test_ship3.name() = "TestShip3";
  test_ship3.type() = ShipType::STYPE_FIGHTER;
  test_ship3.alive() = 1;
  test_ship3.active() = 1;

  // Store ships in database
  putship(test_ship1);
  putship(test_ship2);
  putship(test_ship3);

  // Test that Numships() returns the correct count
  shipnum_t count_after_inserts = Numships();
  assert(count_after_inserts == initial_count + 3);

  // Test that we can retrieve the ships
  auto retrieved_ship1 = getship(1);
  assert(retrieved_ship1.has_value());
  assert(retrieved_ship1.value().number() == 1);
  assert(retrieved_ship1.value().name() == "TestShip1");
  assert(retrieved_ship1.value().type() == ShipType::STYPE_SHUTTLE);

  auto retrieved_ship2 = getship(2);
  assert(retrieved_ship2.has_value());
  assert(retrieved_ship2.value().number() == 2);
  assert(retrieved_ship2.value().name() == "TestShip2");
  assert(retrieved_ship2.value().type() == ShipType::STYPE_CARGO);

  auto retrieved_ship3 = getship(3);
  assert(retrieved_ship3.has_value());
  assert(retrieved_ship3.value().number() == 3);
  assert(retrieved_ship3.value().name() == "TestShip3");
  assert(retrieved_ship3.value().type() == ShipType::STYPE_FIGHTER);

  // Test that retrieving a non-existent ship returns empty optional
  auto non_existent_ship = getship(999);
  assert(!non_existent_ship.has_value());

  // Test that retrieving with invalid ship number returns empty optional
  auto invalid_ship = getship(0);
  assert(!invalid_ship.has_value());

  auto negative_ship = getship(-1);
  assert(!negative_ship.has_value());

  std::println("Numships and ship storage test passed!");
  std::println("Initial count: {}, Final count: {}", initial_count,
               count_after_inserts);
  return 0;
}