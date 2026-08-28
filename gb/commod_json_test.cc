// SPDX-License-Identifier: Apache-2.0

/// \file commod_json_test.cc
/// \brief Unit tests for Commod entity SQLite JSON serialization and
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

  // Initialize database tables - this will create the tbl_commod table
  initialize_schema(db);

  // Create EntityManager for accessing commodities
  EntityManager em(db);

  // Create JsonStore and Repository for initial save
  JsonStore store(db);
  CommodRepository commod_repo(store);

  Commod test_commod{};

  // Initialize some basic fields for testing
  test_commod.id = 42;
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

  // Test Repository::save - stores in SQLite as JSON
  commod_repo.save(test_commod);

  // Test EntityManager::peek_commod - reads from SQLite
  const auto* retrieved_commod = em.peek_commod(commodnum);
  test::expect_ne(retrieved_commod, nullptr);

  // Verify key fields
  test::expect_eq(retrieved_commod->owner, test_commod.owner);
  test::expect_eq(retrieved_commod->governor, test_commod.governor);
  test::expect_eq(retrieved_commod->type, test_commod.type);
  test::expect_eq(retrieved_commod->amount, test_commod.amount);
  test::expect_eq(retrieved_commod->deliver, test_commod.deliver);
  test::expect_eq(retrieved_commod->bid, test_commod.bid);
  test::expect_eq(retrieved_commod->bidder, test_commod.bidder);
  test::expect_eq(retrieved_commod->bidder_gov, test_commod.bidder_gov);
  test::expect_eq(retrieved_commod->star_from, test_commod.star_from);
  test::expect_eq(retrieved_commod->planet_from, test_commod.planet_from);
  test::expect_eq(retrieved_commod->star_to, test_commod.star_to);
  test::expect_eq(retrieved_commod->planet_to, test_commod.planet_to);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println(std::cout, "Commod SQLite JSON storage test passed!");
  return 0;
}