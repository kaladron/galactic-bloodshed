// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <sqlite3.h>

#include <cassert>

int main() {
  // Initialize database using Database class (in-memory for testing)
  Database db(":memory:");

  // Initialize database tables - this will create the tbl_stardata table
  initialize_schema(db);

  stardata test_stardata{};

  // Initialize some basic fields for testing
  test_stardata.numstars = 100;
  test_stardata.ships = 5;
  test_stardata.AP[0] = 10;
  test_stardata.AP[1] = 20;
  test_stardata.VN_hitlist[0] = 3;
  test_stardata.VN_index1[0] = 1;
  test_stardata.VN_index2[0] = 2;
  test_stardata.dummy[0] = 12345;
  test_stardata.dummy[1] = 67890;

  // Test putsdata - stores in SQLite as JSON
  putsdata(&test_stardata);

  // Test getsdata - reads from SQLite
  stardata retrieved_stardata{};
  getsdata(&retrieved_stardata);

  // Verify key fields
  assert(retrieved_stardata.numstars == test_stardata.numstars);
  assert(retrieved_stardata.ships == test_stardata.ships);
  assert(retrieved_stardata.AP[0] == test_stardata.AP[0]);
  assert(retrieved_stardata.AP[1] == test_stardata.AP[1]);
  assert(retrieved_stardata.VN_hitlist[0] == test_stardata.VN_hitlist[0]);
  assert(retrieved_stardata.VN_index1[0] == test_stardata.VN_index1[0]);
  assert(retrieved_stardata.VN_index2[0] == test_stardata.VN_index2[0]);
  assert(retrieved_stardata.dummy[0] == test_stardata.dummy[0]);
  assert(retrieved_stardata.dummy[1] == test_stardata.dummy[1]);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println("stardata SQLite JSON storage test passed!");
  return 0;
}