// SPDX-License-Identifier: Apache-2.0

import gblib;
import std.compat;

#include <cassert>
#include <sqlite3.h>

int main() {
  // Initialize database connection manually for testing  
  int err = sqlite3_open(":memory:", &dbconn);
  if (err) {
    std::printf("Can't open database: %s\n", sqlite3_errmsg(dbconn));
    return 1;
  }
  
  // Initialize database tables - this will create the tbl_race table
  initsqldata();
  
  Race test_race{};
  
  // Initialize some basic fields for testing
  test_race.Playernum = 42;
  strcpy(test_race.name, "TestRace");
  strcpy(test_race.password, "testpass");
  strcpy(test_race.info, "Test race information");
  test_race.IQ = 150;
  test_race.tech = 100.0;
  test_race.governors = 1;
  
  // Test putrace - stores in SQLite as JSON
  putrace(test_race);
  
  // Test getrace - reads from SQLite
  Race retrieved_race = getrace(42);
  
  // Verify key fields
  assert(retrieved_race.Playernum == test_race.Playernum);
  assert(strcmp(retrieved_race.name, test_race.name) == 0);
  assert(retrieved_race.IQ == test_race.IQ);
  assert(retrieved_race.tech == test_race.tech);
  assert(retrieved_race.governors == test_race.governors);
  
  // Clean up
  sqlite3_close(dbconn);
  
  std::printf("Race SQLite storage test passed!\n");
  return 0;
}