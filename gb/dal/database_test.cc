// SPDX-License-Identifier: Apache-2.0

import dallib;
import std.compat;

#include <cassert>

int main() {
  std::println("Testing Database class...");

  // Test 1: Create in-memory database
  {
    Database db(":memory:");
    assert(db.is_open());
    std::println("✓ Can create in-memory database");
  }

  // Test 2: Create file-based database
  {
    const std::string test_db = "/tmp/test_database.db";
    
    // Clean up if exists (ignore errors)
    std::remove(test_db.c_str());
    
    {
      Database db(test_db);
      assert(db.is_open());
      std::println("✓ Can create file-based database");
    }
    
    // Verify file was created
    std::FILE* file = std::fopen(test_db.c_str(), "r");
    assert(file != nullptr);
    std::fclose(file);
    std::println("✓ Database file was created");
    
    // Clean up
    std::remove(test_db.c_str());
  }

  // Test 3: Transaction support
  {
    Database db(":memory:");
    
    // Should not throw
    db.begin_transaction();
    std::println("✓ Can begin transaction");
    
    db.commit();
    std::println("✓ Can commit transaction");
    
    db.begin_transaction();
    db.rollback();
    std::println("✓ Can rollback transaction");
  }

  // Test 4: Move semantics
  {
    Database db1(":memory:");
    assert(db1.is_open());
    
    Database db2(std::move(db1));
    assert(db2.is_open());
    assert(!db1.is_open());
    std::println("✓ Move constructor works");
    
    Database db3(":memory:");
    db3 = std::move(db2);
    assert(db3.is_open());
    assert(!db2.is_open());
    std::println("✓ Move assignment works");
  }

  // Test 5: Destructor closes connection
  {
    Database* db = new Database(":memory:");
    assert(db->is_open());
    delete db;
    std::println("✓ Destructor closes connection");
  }

  std::println("\nAll Database tests passed!");
  return 0;
}
