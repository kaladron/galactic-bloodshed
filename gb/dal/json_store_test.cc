// SPDX-License-Identifier: Apache-2.0

/// \file json_store_test.cc
/// \brief Unit tests for JsonStore JSON serialization and SQLite CRUD
/// operations.

import dallib;
import test;
import std;

int main() {
  std::println(std::cout, "Testing JsonStore class...");

  // Create in-memory database and initialize canonical schema
  Database db(":memory:");
  initialize_schema(db);

  JsonStore store(db);

  // Store and retrieve JSON
  {
    std::string json = R"({"name": "test", "value": 42})";
    bool stored = store.store("tbl_star", 1, json);
    test::expect_true(stored);
    std::println(std::cout, "✓ Can store JSON");

    auto retrieved = store.retrieve("tbl_star", 1);
    test::expect_true(retrieved.has_value());
    test::expect_eq(*retrieved, json);
    std::println(std::cout, "✓ Can retrieve JSON");
  }

  // Update existing entry
  {
    std::string json_v2 = R"({"name": "test", "value": 100})";
    bool stored = store.store("tbl_star", 1, json_v2);
    test::expect_true(stored);

    auto retrieved = store.retrieve("tbl_star", 1);
    test::expect_true(retrieved.has_value());
    test::expect_eq(*retrieved, json_v2);
    std::println(std::cout, "✓ Can update existing entry");
  }

  // Store multiple entries
  {
    store.store("tbl_star", 2, R"({"id": 2})");
    store.store("tbl_star", 3, R"({"id": 3})");
    store.store("tbl_star", 5, R"({"id": 5})");  // Gap at 4

    auto ids = store.list_ids("tbl_star");
    test::expect_eq(ids.size(), 4);
    test::expect_eq(ids[0], 1);
    test::expect_eq(ids[1], 2);
    test::expect_eq(ids[2], 3);
    test::expect_eq(ids[3], 5);
    std::println(std::cout, "✓ Can list all IDs");
  }

  // Find next available ID (gap-finding)
  {
    int next_id = store.find_next_available_id("tbl_star");
    test::expect_eq(next_id, 4);  // Should find the gap
    std::println(std::cout, "✓ Gap-finding returns correct ID (4)");

    // Fill the gap
    store.store("tbl_star", 4, R"({"id": 4})");

    // Now should return 6 (next after max)
    next_id = store.find_next_available_id("tbl_star");
    test::expect_eq(next_id, 6);
    std::println(std::cout,
                 "✓ After filling gap, returns next ID after max (6)");
  }

  // Remove entry
  {
    bool removed = store.remove("tbl_star", 2);
    test::expect_true(removed);

    auto retrieved = store.retrieve("tbl_star", 2);
    test::expect_false(retrieved.has_value());
    std::println(std::cout, "✓ Can remove entry");

    // Gap-finding should now return 2
    int next_id = store.find_next_available_id("tbl_star");
    test::expect_eq(next_id, 2);
    std::println(std::cout, "✓ Gap-finding finds removed entry slot (2)");
  }

  // Multi-key operations (composite keys)
  {
    // Ensure parent star exists for composite planet key (star_id = 10)
    store.store("tbl_star", 10, R"({"star_id": 10, "name": "Sol"})");

    // Store with composite key
    std::vector<std::pair<std::string, KeyValue>> keys = {{"star_id", 10},
                                                          {"planet_order", 3}};
    std::string json = R"({"type": "planet", "name": "Earth"})";

    bool stored = store.store_multi("tbl_planet", keys, json);
    test::expect_true(stored);
    std::println(std::cout, "✓ Can store with composite key");

    // Retrieve with composite key
    auto retrieved = store.retrieve_multi("tbl_planet", keys);
    test::expect_true(retrieved.has_value());
    test::expect_eq(*retrieved, json);
    std::println(std::cout, "✓ Can retrieve with composite key");

    // Store another entry with same star but different planet
    keys = {{"star_id", 10}, {"planet_order", 5}};
    stored = store.store_multi("tbl_planet", keys,
                               R"({"type": "planet", "name": "Mars"})");
    test::expect_true(stored);

    // Verify we can retrieve both
    keys = {{"star_id", 10}, {"planet_order", 3}};
    retrieved = store.retrieve_multi("tbl_planet", keys);
    test::expect_true(retrieved.has_value());
    test::expect_contains(*retrieved, "Earth");

    keys = {{"star_id", 10}, {"planet_order", 5}};
    retrieved = store.retrieve_multi("tbl_planet", keys);
    test::expect_true(retrieved.has_value());
    test::expect_contains(*retrieved, "Mars");
    std::println(std::cout,
                 "✓ Multiple entries with composite keys work correctly");
  }

  // Empty table behavior
  {
    // find_next_available_id should return 1 for empty table (tbl_commod)
    int next_id = store.find_next_available_id("tbl_commod");
    test::expect_eq(next_id, 1);
    std::println(std::cout, "✓ Empty table returns ID 1");

    auto ids = store.list_ids("tbl_commod");
    test::expect_true(ids.empty());
    std::println(std::cout, "✓ Empty table returns empty ID list");
  }

  // Non-existent key lookups
  {
    auto non_existent = store.retrieve("tbl_star", 9999);
    test::expect_false(non_existent.has_value());
    std::println(std::cout, "✓ retrieve on non-existent key returns nullopt");

    std::vector<std::pair<std::string, KeyValue>> non_existent_keys = {
        {"star_id", 999}, {"planet_order", 999}};
    auto non_existent_multi =
        store.retrieve_multi("tbl_planet", non_existent_keys);
    test::expect_false(non_existent_multi.has_value());
    std::println(
        std::cout,
        "✓ retrieve_multi on non-existent composite key returns nullopt");

    // Empty keys vector returns nullopt
    std::vector<std::pair<std::string, KeyValue>> empty_keys;
    auto empty_result = store.retrieve_multi("tbl_planet", empty_keys);
    test::expect_false(empty_result.has_value());
    std::println(std::cout, "✓ retrieve_multi with empty keys returns nullopt");
  }

  // Update overwrite on composite keys
  {
    std::vector<std::pair<std::string, KeyValue>> keys = {{"star_id", 10},
                                                          {"planet_order", 3}};
    std::string updated_json =
        R"({"type": "planet", "name": "Earth v2", "popn": 5000})";

    bool stored = store.store_multi("tbl_planet", keys, updated_json);
    test::expect_true(stored);

    auto retrieved = store.retrieve_multi("tbl_planet", keys);
    test::expect_true(retrieved.has_value());
    test::expect_eq(*retrieved, updated_json);
    std::println(std::cout,
                 "✓ store_multi overwrites existing composite key entry");
  }

  // Transaction rollback with JsonStore
  {
    db.begin_transaction();
    bool stored = store.store("tbl_star", 888, R"({"trans": "temporary"})");
    test::expect_true(stored);
    test::expect_true(store.retrieve("tbl_star", 888).has_value());

    db.rollback();
    auto retrieved = store.retrieve("tbl_star", 888);
    test::expect_false(retrieved.has_value());
    std::println(std::cout,
                 "✓ JsonStore respects database transaction rollback");
  }

  // Database error handling (SqliteError exception)
  {
    test::expect_throws<SqliteError>(
        [&] { store.retrieve("non_existent_table", 1); });
    std::println(std::cout,
                 "✓ SqliteError thrown on non-existent table retrieve");

    test::expect_throws<SqliteError>(
        [&] { store.store("non_existent_table", 1, R"({"test": 1})"); });
    std::println(std::cout, "✓ SqliteError thrown on non-existent table store");
  }

  std::println(std::cout, "\nAll JsonStore tests passed!");
  return 0;
}
