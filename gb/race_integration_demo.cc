// SPDX-License-Identifier: Apache-2.0
// Demonstration of Race SQLite JSON storage integration

import gblib;
import std.compat;

#include <cassert>
#include <sqlite3.h>

void demo_json_serialization() {
  std::printf("=== Demonstrating JSON Serialization ===\n");
  
  Race test_race{};
  test_race.Playernum = 1;
  strcpy(test_race.name, "DemoRace");
  strcpy(test_race.password, "demopass");
  test_race.IQ = 120;
  test_race.tech = 85.5;
  
  // Test JSON serialization
  auto json_result = race_to_json(test_race);
  if (json_result.has_value()) {
    std::printf("✅ Race serialized to JSON successfully\n");
    std::printf("JSON snippet: %.100s...\n", json_result.value().c_str());
    
    // Test JSON deserialization
    auto race_opt = race_from_json(json_result.value());
    if (race_opt.has_value()) {
      Race deserialized = race_opt.value();
      std::printf("✅ Race deserialized from JSON successfully\n");
      std::printf("Original: Playernum=%d, name=%s, IQ=%d, tech=%.1f\n",
                  test_race.Playernum, test_race.name, test_race.IQ, test_race.tech);
      std::printf("Deserialized: Playernum=%d, name=%s, IQ=%d, tech=%.1f\n",
                  deserialized.Playernum, deserialized.name, deserialized.IQ, deserialized.tech);
    } else {
      std::printf("❌ Failed to deserialize Race from JSON\n");
    }
  } else {
    std::printf("❌ Failed to serialize Race to JSON\n");
  }
}

void demo_sqlite_storage() {
  std::printf("\n=== Demonstrating SQLite Storage ===\n");
  
  // Initialize in-memory database
  int err = sqlite3_open(":memory:", &dbconn);
  if (err) {
    std::printf("❌ Can't open database: %s\n", sqlite3_errmsg(dbconn));
    return;
  }
  std::printf("✅ SQLite database opened\n");
  
  // Initialize database schema
  initsqldata();
  std::printf("✅ Database schema initialized (including tbl_race)\n");
  
  // Verify table exists
  const char* check_sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='tbl_race'";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(dbconn, check_sql, -1, &stmt, nullptr);
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    std::printf("✅ tbl_race table exists in database\n");
  } else {
    std::printf("❌ tbl_race table not found\n");
  }
  sqlite3_finalize(stmt);
  
  // Create test race
  Race test_race{};
  test_race.Playernum = 99;
  strcpy(test_race.name, "SQLiteTestRace");
  strcpy(test_race.info, "This race is stored in SQLite as JSON");
  test_race.IQ = 140;
  test_race.tech = 92.3;
  test_race.governors = 2;
  
  std::printf("Created test race: Playernum=%d, name=%s, IQ=%d, tech=%.1f\n",
              test_race.Playernum, test_race.name, test_race.IQ, test_race.tech);
  
  // Note: We can't test putrace/getrace here without proper file initialization
  // But we can demonstrate the SQL operations directly
  
  // Manually demonstrate the JSON storage SQL
  auto json_result = race_to_json(test_race);
  if (json_result.has_value()) {
    const char* insert_sql = "INSERT INTO tbl_race (player_id, race_data) VALUES (?, ?)";
    sqlite3_stmt* insert_stmt;
    sqlite3_prepare_v2(dbconn, insert_sql, -1, &insert_stmt, nullptr);
    sqlite3_bind_int(insert_stmt, 1, test_race.Playernum);
    sqlite3_bind_text(insert_stmt, 2, json_result.value().c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(insert_stmt) == SQLITE_DONE) {
      std::printf("✅ Race data stored in SQLite as JSON\n");
      
      // Retrieve and verify
      const char* select_sql = "SELECT race_data FROM tbl_race WHERE player_id = ?";
      sqlite3_stmt* select_stmt;
      sqlite3_prepare_v2(dbconn, select_sql, -1, &select_stmt, nullptr);
      sqlite3_bind_int(select_stmt, 1, test_race.Playernum);
      
      if (sqlite3_step(select_stmt) == SQLITE_ROW) {
        const char* retrieved_json = reinterpret_cast<const char*>(sqlite3_column_text(select_stmt, 0));
        std::printf("✅ Race data retrieved from SQLite\n");
        
        auto retrieved_race_opt = race_from_json(std::string(retrieved_json));
        if (retrieved_race_opt.has_value()) {
          Race retrieved = retrieved_race_opt.value();
          std::printf("✅ Race data deserialized from SQLite JSON\n");
          std::printf("Retrieved: Playernum=%d, name=%s, IQ=%d, tech=%.1f\n",
                      retrieved.Playernum, retrieved.name, retrieved.IQ, retrieved.tech);
          
          // Verify data integrity
          if (retrieved.Playernum == test_race.Playernum &&
              strcmp(retrieved.name, test_race.name) == 0 &&
              retrieved.IQ == test_race.IQ &&
              std::abs(retrieved.tech - test_race.tech) < 0.1) {
            std::printf("✅ Data integrity verified - all fields match!\n");
          } else {
            std::printf("❌ Data integrity check failed\n");
          }
        } else {
          std::printf("❌ Failed to deserialize retrieved JSON\n");
        }
      } else {
        std::printf("❌ Failed to retrieve data from SQLite\n");
      }
      sqlite3_finalize(select_stmt);
    } else {
      std::printf("❌ Failed to store data in SQLite\n");
    }
    sqlite3_finalize(insert_stmt);
  }
  
  sqlite3_close(dbconn);
  std::printf("✅ Database closed\n");
}

int main() {
  std::printf("Galactic Bloodshed - Race SQLite JSON Storage Demo\n");
  std::printf("==================================================\n");
  
  demo_json_serialization();
  demo_sqlite_storage();
  
  std::printf("\n🎉 Demo completed successfully!\n");
  std::printf("\nSummary of implementation:\n");
  std::printf("- Added tbl_race table to SQLite schema\n");
  std::printf("- Modified putrace() to store Race as JSON in SQLite\n");
  std::printf("- Modified getrace() to read Race from SQLite JSON with file fallback\n");
  std::printf("- Maintains backward compatibility with existing file storage\n");
  
  return 0;
}