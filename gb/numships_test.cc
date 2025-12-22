// SPDX-License-Identifier: Apache-2.0

import dallib;
import dallib;
import gblib;
import std.compat;

#include <cassert>

int main() {
  // Initialize database using Database class (in-memory for testing)
  Database db(":memory:");

  // Initialize database tables - this will create the ship tables
  initialize_schema(db);

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

  // Store ships in database using Repository (DAL layer)
  JsonStore store(db);
  ShipRepository ship_repo(store);
  ship_repo.save(test_ship1);
  ship_repo.save(test_ship2);
  ship_repo.save(test_ship3);

  // Create EntityManager
  EntityManager entity_manager(db);

  // Get initial count (should be 3)
  shipnum_t initial_count = entity_manager.num_ships();

  // Test that num_ships() returns the correct count
  shipnum_t count_after_inserts = entity_manager.num_ships();
  assert(count_after_inserts == 3);

  // Test that we can retrieve the ships
  const auto* retrieved_ship1 = entity_manager.peek_ship(1);
  assert(retrieved_ship1);
  assert(retrieved_ship1->number() == 1);
  assert(retrieved_ship1->name() == "TestShip1");
  assert(retrieved_ship1->type() == ShipType::STYPE_SHUTTLE);

  const auto* retrieved_ship2 = entity_manager.peek_ship(2);
  assert(retrieved_ship2);
  assert(retrieved_ship2->number() == 2);
  assert(retrieved_ship2->name() == "TestShip2");
  assert(retrieved_ship2->type() == ShipType::STYPE_CARGO);

  const auto* retrieved_ship3 = entity_manager.peek_ship(3);
  assert(retrieved_ship3);
  assert(retrieved_ship3->number() == 3);
  assert(retrieved_ship3->name() == "TestShip3");
  assert(retrieved_ship3->type() == ShipType::STYPE_FIGHTER);

  // Test that retrieving a non-existent ship returns nullptr
  const auto* non_existent_ship = entity_manager.peek_ship(999);
  assert(!non_existent_ship);

  // Test that retrieving with invalid ship number returns nullptr
  const auto* invalid_ship = entity_manager.peek_ship(0);
  assert(!invalid_ship);

  const auto* negative_ship = entity_manager.peek_ship(-1);
  assert(!negative_ship);

  std::println("Numships and ship storage test passed!");
  std::println("Initial count: {}, Final count: {}", initial_count,
               count_after_inserts);
  return 0;
}