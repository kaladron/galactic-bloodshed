// SPDX-License-Identifier: Apache-2.0

/// \file block_json_test.cc
/// \brief Unit tests for Block entity SQLite JSON serialization and
/// EntityManager integration.

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import gb.turn;
import test;
import std;

int main() {
  // Initialize database using Database class (in-memory for testing)
  Database db(":memory:");

  // Initialize database tables - this will create the tbl_block table
  initialize_schema(db);

  std::flat_map<player_t, block> test_blocks;
  for (player_t p : all_players()) {
    test_blocks[p] = block{.Playernum = p};
  }

  // Initialize some test data for a few players
  test_blocks[1].Playernum = 1;
  test_blocks[1].name = "TestPlayer1";
  test_blocks[1].motto = "TestMotto1";
  test_blocks[1].invited = {player_t{1}, player_t{2}, player_t{6}};
  test_blocks[1].pledged = {player_t{2}, player_t{3}, player_t{7}};
  test_blocks[1].systems_owned = 5;
  test_blocks[1].VPs = 1000;
  test_blocks[1].money = 50000;

  test_blocks[2].Playernum = 2;
  test_blocks[2].name = "TestPlayer2";
  test_blocks[2].motto = "TestMotto2";
  test_blocks[2].invited = {player_t{1}, player_t{3}, player_t{4}};
  test_blocks[2].pledged = {player_t{1}, player_t{6}, player_t{9}};
  test_blocks[2].systems_owned = 3;
  test_blocks[2].VPs = 800;
  test_blocks[2].money = 30000;

  // Test EntityManager - stores and retrieves block data
  // First save using repository
  JsonStore store(db);
  RaceRepository race_repo(store);
  BlockRepository block_repo(store);
  for (player_t p : all_players()) {
    Race r{};
    r.Playernum = p;
    race_repo.save(r);
    block_repo.save(test_blocks[p]);
  }

  // Now use EntityManager to retrieve and verify
  EntityManager em(db);
  std::flat_map<player_t, block> retrieved_blocks;
  for (player_t p : all_players()) {
    const auto* block_ptr = em.peek_block(p);
    test::expect_ne(block_ptr, nullptr);  // Should exist now
    retrieved_blocks[p] = *block_ptr;
  }

  // Verify key fields for first player
  test::expect_eq(retrieved_blocks[1].Playernum, test_blocks[1].Playernum);
  test::expect_eq(retrieved_blocks[1].name, test_blocks[1].name);
  test::expect_eq(retrieved_blocks[1].motto, test_blocks[1].motto);
  test::expect_eq(retrieved_blocks[1].invited, test_blocks[1].invited);
  test::expect_eq(retrieved_blocks[1].pledged, test_blocks[1].pledged);
  test::expect_eq(retrieved_blocks[1].systems_owned,
                  test_blocks[1].systems_owned);
  test::expect_eq(retrieved_blocks[1].VPs, test_blocks[1].VPs);
  test::expect_eq(retrieved_blocks[1].money, test_blocks[1].money);

  // Verify key fields for second player
  test::expect_eq(retrieved_blocks[2].Playernum, test_blocks[2].Playernum);
  test::expect_eq(retrieved_blocks[2].name, test_blocks[2].name);
  test::expect_eq(retrieved_blocks[2].motto, test_blocks[2].motto);
  test::expect_eq(retrieved_blocks[2].invited, test_blocks[2].invited);
  test::expect_eq(retrieved_blocks[2].pledged, test_blocks[2].pledged);
  test::expect_eq(retrieved_blocks[2].systems_owned,
                  test_blocks[2].systems_owned);
  test::expect_eq(retrieved_blocks[2].VPs, test_blocks[2].VPs);
  test::expect_eq(retrieved_blocks[2].money, test_blocks[2].money);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println(std::cout, "block SQLite JSON storage test passed!");
  return 0;
}