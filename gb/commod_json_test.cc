// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <sqlite3.h>

#include <cassert>

int main() {
  // Initialize database using Database class (in-memory for testing)
  Database db(":memory:");

  // Initialize database tables - this will create the tbl_commod_json table
  initialize_schema(db);

  Commod test_commod{};

  // Initialize some basic fields for testing
  test_commod.owner = 1;
  test_commod.governor = 2;
  test_commod.type = CommodType::RESOURCE;
  test_commod.amount = 500;
  test_commod.deliver = true;
  test_commod.bid = 100;
  test_commod.bidder = 3;
  test_commod.bidder_gov = 4;
  test_commod.star_from = 10;
  test_commod.planet_from = 2;
  test_commod.star_to = 15;
  test_commod.planet_to = 3;

  int commodnum = 42;

  // Test putcommod - stores in SQLite as JSON
  putcommod(test_commod, commodnum);

  // Test getcommod - reads from SQLite
  Commod retrieved_commod = getcommod(commodnum);

  // Verify key fields
  assert(retrieved_commod.owner == test_commod.owner);
  assert(retrieved_commod.governor == test_commod.governor);
  assert(retrieved_commod.type == test_commod.type);
  assert(retrieved_commod.amount == test_commod.amount);
  assert(retrieved_commod.deliver == test_commod.deliver);
  assert(retrieved_commod.bid == test_commod.bid);
  assert(retrieved_commod.bidder == test_commod.bidder);
  assert(retrieved_commod.bidder_gov == test_commod.bidder_gov);
  assert(retrieved_commod.star_from == test_commod.star_from);
  assert(retrieved_commod.planet_from == test_commod.planet_from);
  assert(retrieved_commod.star_to == test_commod.star_to);
  assert(retrieved_commod.planet_to == test_commod.planet_to);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println("Commod SQLite JSON storage test passed!");
  return 0;
}