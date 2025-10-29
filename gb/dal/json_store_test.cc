// SPDX-License-Identifier: Apache-2.0

#include <cassert>
#include <sqlite3.h>

import dallib;
import std.compat;

int main() {
  std::println("Testing JsonStore class...");

  // Create in-memory database and initialize a test table
  Database db(":memory:");
  
  // Create a test table with id and data columns
  const char* create_sql = R"(
    CREATE TABLE test_table (
      id INTEGER PRIMARY KEY,
      data TEXT NOT NULL
    )
  )";
  
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db.connection(), create_sql, -1, &stmt, nullptr);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  JsonStore store(db);

  // Test 1: Store and retrieve JSON
  {
    std::string json = R"({"name": "test", "value": 42})";
    bool stored = store.store("test_table", 1, json);
    assert(stored);
    std::println("✓ Can store JSON");

    auto retrieved = store.retrieve("test_table", 1);
    assert(retrieved.has_value());
    assert(*retrieved == json);
    std::println("✓ Can retrieve JSON");
  }

  // Test 2: Update existing entry
  {
    std::string json_v2 = R"({"name": "test", "value": 100})";
    bool stored = store.store("test_table", 1, json_v2);
    assert(stored);

    auto retrieved = store.retrieve("test_table", 1);
    assert(retrieved.has_value());
    assert(*retrieved == json_v2);
    std::println("✓ Can update existing entry");
  }

  // Test 3: Store multiple entries
  {
    store.store("test_table", 2, R"({"id": 2})");
    store.store("test_table", 3, R"({"id": 3})");
    store.store("test_table", 5, R"({"id": 5})");  // Gap at 4

    auto ids = store.list_ids("test_table");
    assert(ids.size() == 4);
    assert(ids[0] == 1);
    assert(ids[1] == 2);
    assert(ids[2] == 3);
    assert(ids[3] == 5);
    std::println("✓ Can list all IDs");
  }

  // Test 4: Find next available ID (gap-finding)
  {
    int next_id = store.find_next_available_id("test_table");
    assert(next_id == 4);  // Should find the gap
    std::println("✓ Gap-finding returns correct ID (4)");

    // Fill the gap
    store.store("test_table", 4, R"({"id": 4})");
    
    // Now should return 6 (next after max)
    next_id = store.find_next_available_id("test_table");
    assert(next_id == 6);
    std::println("✓ After filling gap, returns next ID after max (6)");
  }

  // Test 5: Remove entry
  {
    bool removed = store.remove("test_table", 2);
    assert(removed);

    auto retrieved = store.retrieve("test_table", 2);
    assert(!retrieved.has_value());
    std::println("✓ Can remove entry");

    // Gap-finding should now return 2
    int next_id = store.find_next_available_id("test_table");
    assert(next_id == 2);
    std::println("✓ Gap-finding finds removed entry slot (2)");
  }

  // Test 6: Multi-key operations (composite keys)
  {
    // Create a table with composite keys (like Planet: star_id, planet_order)
    const char* create_multi_sql = R"(
      CREATE TABLE multi_key_table (
        star_id INTEGER NOT NULL,
        planet_order INTEGER NOT NULL,
        data TEXT NOT NULL,
        PRIMARY KEY (star_id, planet_order)
      )
    )";
    
    sqlite3_prepare_v2(db.connection(), create_multi_sql, -1, &stmt, nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Store with composite key
    std::vector<std::pair<std::string, int>> keys = {
        {"star_id", 10}, {"planet_order", 3}};
    std::string json = R"({"type": "planet", "name": "Earth"})";
    
    bool stored = store.store_multi("multi_key_table", keys, json);
    assert(stored);
    std::println("✓ Can store with composite key");

    // Retrieve with composite key
    auto retrieved = store.retrieve_multi("multi_key_table", keys);
    assert(retrieved.has_value());
    assert(*retrieved == json);
    std::println("✓ Can retrieve with composite key");

    // Store another entry with same star but different planet
    keys = {{"star_id", 10}, {"planet_order", 5}};
    stored = store.store_multi("multi_key_table", keys,
                                R"({"type": "planet", "name": "Mars"})");
    assert(stored);

    // Verify we can retrieve both
    keys = {{"star_id", 10}, {"planet_order", 3}};
    retrieved = store.retrieve_multi("multi_key_table", keys);
    assert(retrieved.has_value());
    assert(retrieved->find("Earth") != std::string::npos);

    keys = {{"star_id", 10}, {"planet_order", 5}};
    retrieved = store.retrieve_multi("multi_key_table", keys);
    assert(retrieved.has_value());
    assert(retrieved->find("Mars") != std::string::npos);
    std::println("✓ Multiple entries with composite keys work correctly");
  }

  // Test 7: Empty table behavior
  {
    // Create empty table
    const char* create_empty_sql = R"(
      CREATE TABLE empty_table (
        id INTEGER PRIMARY KEY,
        data TEXT NOT NULL
      )
    )";
    
    sqlite3_prepare_v2(db.connection(), create_empty_sql, -1, &stmt, nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // find_next_available_id should return 1 for empty table
    int next_id = store.find_next_available_id("empty_table");
    assert(next_id == 1);
    std::println("✓ Empty table returns ID 1");

    auto ids = store.list_ids("empty_table");
    assert(ids.empty());
    std::println("✓ Empty table returns empty ID list");
  }

  std::println("\nAll JsonStore tests passed!");
  return 0;
}
