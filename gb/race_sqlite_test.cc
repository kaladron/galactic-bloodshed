// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <sqlite3.h>

#include <cassert>

int main() {
  // Initialize database using common Sql class (in-memory for testing)
  Sql db(":memory:");

  // Initialize database tables - this will create the tbl_race table
  initsqldata();

  Race test_race{};

  // Initialize some basic fields for testing
  test_race.Playernum = 42;
  test_race.name = "TestRace";
  test_race.password = "testpass";
  test_race.info = "Test race information";
  test_race.IQ = 150;
  test_race.tech = 100.0;
  test_race.governors = 1;

  // Test putrace - stores in SQLite as JSON
  putrace(test_race);

  // Test getrace - reads from SQLite
  Race retrieved_race = getrace(42);

  // Verify key fields
  assert(retrieved_race.Playernum == test_race.Playernum);
  assert(retrieved_race.name == test_race.name);
  assert(retrieved_race.IQ == test_race.IQ);
  assert(retrieved_race.tech == test_race.tech);
  assert(retrieved_race.governors == test_race.governors);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println("Race SQLite storage test passed!");
  return 0;
}