// SPDX-License-Identifier: Apache-2.0

/// \file star_sqlite_test.cc
/// \brief Unit tests for Star SQLite table persistence and round-trip
/// verification.

import dallib;
import gblib;
import test;
import std;

int main() {
  // CRITICAL: Always create in-memory database BEFORE calling
  // initialize_schema()
  Database db(":memory:");

  // Initialize database tables - this creates all required tables
  initialize_schema(db);

  star_struct test_star{};

  // Initialize scalar fields
  test_star.ships = 42;
  test_star.name = "TestStar";
  test_star.xpos = 100.5;
  test_star.ypos = 200.75;
  test_star.stability = 10;
  test_star.nova_stage = 0;
  test_star.temperature = 15;
  test_star.gravity = 1.0;
  test_star.star_id = 1;

  // Initialize governor array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_star.governor[i] = i + 1;
  }

  // Initialize AP array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test_star.AP[i] = i * 100;
  }

  // Initialize explored and inhabited bitmasks
  test_star.explored = 0b101010;
  test_star.inhabited = 0b110011;

  // Initialize planet names - now using vector
  for (int i = 0; i < 5; i++) {
    test_star.pnames.push_back(std::format("Planet{}", i));
  }

  // Create Star object from star_struct
  Star test_star_obj(test_star);

  // Use Repository to save - this is how new objects are created
  JsonStore store(db);
  StarRepository star_repo(store);
  star_repo.save(test_star_obj);

  // Create EntityManager to test retrieval
  EntityManager em(db);

  // Test EntityManager peek - reads from SQLite via cache
  const auto* retrieved_star_ptr = em.peek_star(1);
  test::expect_ne(retrieved_star_ptr, nullptr);
  star_struct retrieved = retrieved_star_ptr->get_struct();

  // Verify scalar fields
  test::expect_eq(retrieved.ships, test_star.ships);
  test::expect_eq(retrieved.name, test_star.name);
  test::expect_eq(retrieved.xpos, test_star.xpos);
  test::expect_eq(retrieved.ypos, test_star.ypos);
  test::expect_eq(retrieved.pnames.size(), test_star.pnames.size());
  test::expect_eq(retrieved.stability, test_star.stability);
  test::expect_eq(retrieved.nova_stage, test_star.nova_stage);
  test::expect_eq(retrieved.temperature, test_star.temperature);
  test::expect_eq(retrieved.gravity, test_star.gravity);

  // Verify governor array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test::expect_eq(retrieved.governor[i], test_star.governor[i]);
  }

  // Verify AP array
  for (int i = 0; i < MAXPLAYERS; i++) {
    test::expect_eq(retrieved.AP[i], test_star.AP[i]);
  }

  // Verify bitmasks
  test::expect_eq(retrieved.explored, test_star.explored);
  test::expect_eq(retrieved.inhabited, test_star.inhabited);

  // Verify planet names
  for (std::size_t i = 0; i < test_star.pnames.size(); i++) {
    test::expect_eq(retrieved.pnames[i], test_star.pnames[i]);
  }

  // Database connection will be cleaned up automatically by Database destructor

  std::println(std::cout, "Star SQLite storage test passed!");
  return 0;
}
