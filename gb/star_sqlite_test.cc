// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <sqlite3.h>

#include <cassert>
#include <cstring>

int main() {
  // CRITICAL: Always create in-memory database BEFORE calling
  // initialize_schema()
  Database db(":memory:");

  // Initialize database tables - this creates all required tables
  initialize_schema(db);

  star_struct test_star{};

  // Initialize scalar fields
  test_star.ships = 42;
  test_star.name = "TestStar";
  test_star.xpos = 100.5;
  test_star.ypos = 200.75;
  test_star.stability = 10;
  test_star.nova_stage = 0;
  test_star.temperature = 15;
  test_star.gravity = 1.0;
  test_star.star_id = 1;

  // Initialize governor array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_star.governor[i] = i + 1;
  }

  // Initialize AP array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_star.AP[i] = i * 100;
  }

  // Initialize explored and inhabited bitmasks
  test_star.explored = 0b101010;
  test_star.inhabited = 0b110011;

  // Initialize planet names - now using vector
  for (int i = 0; i < 5; i++) {
    test_star.pnames.push_back(std::format("Planet{}", i));
  }

  // Create Star object from star_struct
  Star test_star_obj(test_star);

  // Test putstar - stores in SQLite as JSON
  putstar(test_star_obj, 1);

  // Test getstar - reads from SQLite
  Star retrieved_star = getstar(1);
  star_struct retrieved = retrieved_star.get_struct();

  // Verify scalar fields
  assert(retrieved.ships == test_star.ships);
  assert(retrieved.name == test_star.name);
  assert(retrieved.xpos == test_star.xpos);
  assert(retrieved.ypos == test_star.ypos);
  assert(retrieved.pnames.size() == test_star.pnames.size());
  assert(retrieved.stability == test_star.stability);
  assert(retrieved.nova_stage == test_star.nova_stage);
  assert(retrieved.temperature == test_star.temperature);
  assert(retrieved.gravity == test_star.gravity);

  // Verify governor array
  for (int i = 0; i < MAXPLAYERS; i++) {
    assert(retrieved.governor[i] == test_star.governor[i]);
  }

  // Verify AP array
  for (int i = 0; i < MAXPLAYERS; i++) {
    assert(retrieved.AP[i] == test_star.AP[i]);
  }

  // Verify bitmasks
  assert(retrieved.explored == test_star.explored);
  assert(retrieved.inhabited == test_star.inhabited);

  // Verify planet names
  for (size_t i = 0; i < test_star.pnames.size(); i++) {
    assert(retrieved.pnames[i] == test_star.pnames[i]);
  }

  // Database connection will be cleaned up automatically by Sql destructor

  std::println("Star SQLite storage test passed!");
  return 0;
}
