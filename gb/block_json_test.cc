// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import std.compat;

#include <sqlite3.h>

#include <cassert>
#include <cstring>

int main() {
  // Initialize database using Database class (in-memory for testing)
  Database db(":memory:");

  // Initialize database tables - this will create the tbl_block table
  initialize_schema(db);

  block test_blocks[MAXPLAYERS];

  // Initialize some test data for a few players
  test_blocks[0].Playernum = 1;
  test_blocks[0].name = "TestPlayer1";
  test_blocks[0].motto = "TestMotto1";
  test_blocks[0].invite = 0x123;
  test_blocks[0].pledge = 0x456;
  test_blocks[0].atwar = 0x789;
  test_blocks[0].allied = 0xABC;
  test_blocks[0].next = 2;
  test_blocks[0].systems_owned = 5;
  test_blocks[0].VPs = 1000;
  test_blocks[0].money = 50000;

  test_blocks[1].Playernum = 2;
  test_blocks[1].name = "TestPlayer2";
  test_blocks[1].motto = "TestMotto2";
  test_blocks[1].invite = 0xDEF;
  test_blocks[1].pledge = 0x321;
  test_blocks[1].atwar = 0x654;
  test_blocks[1].allied = 0x987;
  test_blocks[1].next = 3;
  test_blocks[1].systems_owned = 3;
  test_blocks[1].VPs = 800;
  test_blocks[1].money = 30000;

  // Initialize remaining blocks to empty
  for (int i = 2; i < MAXPLAYERS; i++) {
    test_blocks[i] = block{};
  }

  // Test Putblock - stores in SQLite as JSON
  Putblock(test_blocks);

  // Test Getblock - reads from SQLite
  block retrieved_blocks[MAXPLAYERS];
  Getblock(retrieved_blocks);

  // Verify key fields for first player
  assert(retrieved_blocks[0].Playernum == test_blocks[0].Playernum);
  assert(retrieved_blocks[0].name == test_blocks[0].name);
  assert(retrieved_blocks[0].motto == test_blocks[0].motto);
  assert(retrieved_blocks[0].invite == test_blocks[0].invite);
  assert(retrieved_blocks[0].pledge == test_blocks[0].pledge);
  assert(retrieved_blocks[0].atwar == test_blocks[0].atwar);
  assert(retrieved_blocks[0].allied == test_blocks[0].allied);
  assert(retrieved_blocks[0].next == test_blocks[0].next);
  assert(retrieved_blocks[0].systems_owned == test_blocks[0].systems_owned);
  assert(retrieved_blocks[0].VPs == test_blocks[0].VPs);
  assert(retrieved_blocks[0].money == test_blocks[0].money);

  // Verify key fields for second player
  assert(retrieved_blocks[1].Playernum == test_blocks[1].Playernum);
  assert(retrieved_blocks[1].name == test_blocks[1].name);
  assert(retrieved_blocks[1].motto == test_blocks[1].motto);
  assert(retrieved_blocks[1].invite == test_blocks[1].invite);
  assert(retrieved_blocks[1].pledge == test_blocks[1].pledge);
  assert(retrieved_blocks[1].atwar == test_blocks[1].atwar);
  assert(retrieved_blocks[1].allied == test_blocks[1].allied);
  assert(retrieved_blocks[1].next == test_blocks[1].next);
  assert(retrieved_blocks[1].systems_owned == test_blocks[1].systems_owned);
  assert(retrieved_blocks[1].VPs == test_blocks[1].VPs);
  assert(retrieved_blocks[1].money == test_blocks[1].money);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println("block SQLite JSON storage test passed!");
  return 0;
}