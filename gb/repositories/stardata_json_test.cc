// SPDX-License-Identifier: Apache-2.0

/// \file stardata_json_test.cc
/// \brief Unit tests for universe_struct SQLite JSON serialization and
/// EntityManager integration.

import dallib;
import gb.entities;
import gb.repositories;
import gb.services;
import test;
import std;

int main() {
  // Initialize database using Database class (in-memory for testing)
  Database db(":memory:");

  // Initialize database tables - this will create the tbl_universe table
  initialize_schema(db);

  universe_struct test_stardata{};

  // Initialize some basic fields for testing
  test_stardata.set_AP(1, 10);
  test_stardata.set_AP(2, 20);
  test_stardata.vn_target(1) = VnTargetRecord{
      .hits = 3, .primary_star = starnum_t{1}, .secondary_star = starnum_t{2}};

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
  test::expect_eq(retrieved->get_AP(1), test_stardata.get_AP(1));
  test::expect_eq(retrieved->get_AP(2), test_stardata.get_AP(2));
  test::expect_eq(retrieved->vn_hits(1), test_stardata.vn_hits(1));
  test::expect_eq(retrieved->vn_target(1).primary_star,
                  test_stardata.vn_target(1).primary_star);
  test::expect_eq(retrieved->vn_target(1).secondary_star,
                  test_stardata.vn_target(1).secondary_star);

  // Database connection will be cleaned up automatically by Sql destructor

  std::println(std::cout, "universe_struct SQLite JSON storage test passed!");
  return 0;
}