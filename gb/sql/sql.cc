// SPDX-License-Identifier: Apache-2.0

/*  disk input/output routines & msc stuff
 *    all read routines lock the data they just accessed (the file is not
 *    closed).  write routines close and thus unlock that area.
 */

module;

#include <sqlite3.h>

#include <cstdio>

#include "gb/sql/dbdecl.h"

module gblib;

import std.compat;

namespace {
// Apply SQLite strict mode settings - internal implementation detail
void apply_sqlite_strict_mode(sqlite3* db) {
  char* err_msg = nullptr;

  // Enable foreign key constraints
  int err =
      sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQLite PRAGMA foreign_keys failed: {}", err_msg);
    sqlite3_free(err_msg);
  }

  // Enable WAL mode for better concurrency and performance
  err = sqlite3_exec(db, "PRAGMA journal_mode = WAL;", nullptr, nullptr,
                     &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQLite PRAGMA journal_mode failed: {}", err_msg);
    sqlite3_free(err_msg);
  }

  // Set synchronous to NORMAL instead of FULL for better performance
  err = sqlite3_exec(db, "PRAGMA synchronous = NORMAL;", nullptr, nullptr,
                     &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQLite PRAGMA synchronous failed: {}", err_msg);
    sqlite3_free(err_msg);
  }

  // Enable case-sensitive LIKE for more predictable behavior
  err = sqlite3_exec(db, "PRAGMA case_sensitive_like = ON;", nullptr, nullptr,
                     &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQLite PRAGMA case_sensitive_like failed: {}",
                 err_msg);
    sqlite3_free(err_msg);
  }

  // Enable recursive triggers
  err = sqlite3_exec(db, "PRAGMA recursive_triggers = ON;", nullptr, nullptr,
                     &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQLite PRAGMA recursive_triggers failed: {}",
                 err_msg);
    sqlite3_free(err_msg);
  }

  // Set cache size to 64MB for better performance
  err = sqlite3_exec(db, "PRAGMA cache_size = -64000;", nullptr, nullptr,
                     &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQLite PRAGMA cache_size failed: {}", err_msg);
    sqlite3_free(err_msg);
  }

  // Store temporary tables in memory for performance
  err = sqlite3_exec(db, "PRAGMA temp_store = MEMORY;", nullptr, nullptr,
                     &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQLite PRAGMA temp_store failed: {}", err_msg);
    sqlite3_free(err_msg);
  }

  // Enable secure delete to prevent data recovery
  err = sqlite3_exec(db, "PRAGMA secure_delete = ON;", nullptr, nullptr,
                     &err_msg);
  if (err != SQLITE_OK) {
    std::println(stderr, "SQLite PRAGMA secure_delete failed: {}", err_msg);
    sqlite3_free(err_msg);
  }
}

// Common initialization logic for both constructors
void initialize_database(const std::string& db_path) {
  int err = sqlite3_open(db_path.c_str(), &dbconn);
  if (err) {
    std::println(stderr, "Can't open database: {0}", sqlite3_errmsg(dbconn));
    exit(0);
  }

  // Apply SQLite strict mode settings
  apply_sqlite_strict_mode(dbconn);

  open_files();
}
}  // namespace

Sql::Sql() { initialize_database(PKGSTATEDIR "gb.db"); }

Sql::Sql(const std::string& db_path) { initialize_database(db_path); }

Sql::~Sql() {
  close_files();
  if (dbconn) {
    // Use close_v2 which waits for all prepared statements to be finalized
    int rc = sqlite3_close_v2(dbconn);
    if (rc != SQLITE_OK) {
      std::println(stderr, "SQLite close failed: {}", sqlite3_errmsg(dbconn));
    }
    dbconn = nullptr;
  }
}
