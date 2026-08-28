// SPDX-License-Identifier: Apache-2.0

/// \file block_json_test.cc
/// \brief Unit tests for Block entity SQLite JSON serialization and
/// EntityManager integration.

import dallib;
import gb.entities;
import gb.services;
import gb.turn;
import test;
import std;

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

  // Initialize remaining blocks to empty (but with Player num set!)
  for (int i = 2; i < MAXPLAYERS; i++) {
    test_blocks[i] = block{};
    test_blocks[i].Playernum = i + 1;  // CRITICAL: Set Playernum for ID
  }

  // Test EntityManager - stores and retrieves block data
  // First save using repository
  JsonStore store(db);
  BlockRepository block_repo(store);
  for (int i = 0; i < MAXPLAYERS; i++) {
    block_repo.save(test_blocks[i]);
  }

  // Now use EntityManager to retrieve and verify
  EntityManager em(db);
  block retrieved_blocks[MAXPLAYERS];
  for (int i = 0; i < MAXPLAYERS; i++) {
    const auto* block_ptr = em.peek_block(blocknum_t{i + 1});
    test::expect_ne(block_ptr, nullptr);  // Should exist now
    retrieved_blocks[i] = *block_ptr;
  }

  // Verify key fields for first player
  test::expect_eq(retrieved_blocks[0].Playernum, test_blocks[0].Playernum);
  test::expect_eq(retrieved_blocks[0].name, test_blocks[0].name);
  test::expect_eq(retrieved_blocks[0].motto, test_blocks[0].motto);
  test::expect_eq(retrieved_blocks[0].invite, test_blocks[0].invite);
  test::expect_eq(retrieved_blocks[0].pledge, test_blocks[0].pledge);
  test::expect_eq(retrieved_blocks[0].atwar, test_blocks[0].atwar);
  test::expect_eq(retrieved_blocks[0].allied, test_blocks[0].allied);
  test::expect_eq(retrieved_blocks[0].next, test_blocks[0].next);
  test::expect_eq(retrieved_blocks[0].systems_owned,
                  test_blocks[0].systems_owned);
  test::expect_eq(retrieved_blocks[0].VPs, test_blocks[0].VPs);
  test::expect_eq(retrieved_blocks[0].money, test_blocks[0].money);

  // Verify key fields for second player
  test::expect_eq(retrieved_blocks[1].Playernum, test_blocks[1].Playernum);
  test::expect_eq(retrieved_blocks[1].name, test_blocks[1].name);
  test::expect_eq(retrieved_blocks[1].motto, test_blocks[1].motto);
  test::expect_eq(retrieved_blocks[1].invite, test_blocks[1].invite);
  test::expect_eq(retrieved_blocks[1].pledge, test_blocks[1].pledge);
  test::expect_eq(retrieved_blocks[1].atwar, test_blocks[1].atwar);
  test::expect_eq(retrieved_blocks[1].allied, test_blocks[1].allied);
  test::expect_eq(retrieved_blocks[1].next, test_blocks[1].next);
  test::expect_eq(retrieved_blocks[1].systems_owned,
                  test_blocks[1].systems_owned);
  test::expect_eq(retrieved_blocks[1].VPs, test_blocks[1].VPs);
  test::expect_eq(retrieved_blocks[1].money, test_blocks[1].money);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println(std::cout, "block SQLite JSON storage test passed!");
  return 0;
}