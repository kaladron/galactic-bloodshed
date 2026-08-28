// SPDX-License-Identifier: Apache-2.0

/// \file stardata_json_test.cc
/// \brief Unit tests for universe_struct SQLite JSON serialization and
/// EntityManager integration.

import dallib;
import gblib;
import test;
import std;

int main() {
  // Initialize database using Database class (in-memory for testing)
  Database db(":memory:");

  // Initialize database tables - this will create the tbl_universe table
  initialize_schema(db);

  universe_struct test_stardata{};
  test_stardata.id = 1;  // CRITICAL: Universe is a singleton with id=1

  // Initialize some basic fields for testing
  test_stardata.numstars = 100;
  test_stardata.ships = 5;
  test_stardata.AP[0] = 10;
  test_stardata.AP[1] = 20;
  test_stardata.VN_hitlist[0] = 3;
  test_stardata.VN_index1[0] = 1;
  test_stardata.VN_index2[0] = 2;

  // Test EntityManager - stores and retrieves universe data
  // First save using repository to create the database record
  JsonStore store(db);
  UniverseRepository universe_repo(store);
  universe_repo.save(test_stardata);

  // Now use EntityManager to retrieve and verify
  EntityManager em(db);
  const auto* retrieved = em.peek_universe();
  test::expect_ne(retrieved, nullptr);

  // Verify key fields
  test::expect_eq(retrieved->numstars, test_stardata.numstars);
  test::expect_eq(retrieved->ships, test_stardata.ships);
  test::expect_eq(retrieved->AP[0], test_stardata.AP[0]);
  test::expect_eq(retrieved->AP[1], test_stardata.AP[1]);
  test::expect_eq(retrieved->VN_hitlist[0], test_stardata.VN_hitlist[0]);
  test::expect_eq(retrieved->VN_index1[0], test_stardata.VN_index1[0]);
  test::expect_eq(retrieved->VN_index2[0], test_stardata.VN_index2[0]);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println(std::cout, "universe_struct SQLite JSON storage test passed!");
  return 0;
}