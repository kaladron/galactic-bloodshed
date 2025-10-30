// SPDX-License-Identifier: Apache-2.0

module;

import std.compat;

#include <sqlite3.h>

module dallib;

// TEMPORARY: Global dbconn for backward compatibility during transition
// This will be removed in Phase 4 when all code uses Database class
sqlite3* dbconn = nullptr;

namespace {
// Apply SQLite pragmas for strict mode (from existing apply_sqlite_strict_mode)
void apply_pragmas(sqlite3* db) {
  const char* pragmas[] = {
      "PRAGMA foreign_keys = ON;",
      "PRAGMA journal_mode = WAL;",
      "PRAGMA synchronous = NORMAL;",
      "PRAGMA temp_store = MEMORY;",
      "PRAGMA mmap_size = 30000000000;",
      "PRAGMA page_size = 4096;",
      "PRAGMA cache_size = -64000;",  // 64MB cache
  };

  for (const char* pragma : pragmas) {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, pragma, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
      std::string error = errmsg ? errmsg : "Unknown error";
      sqlite3_free(errmsg);
      throw std::runtime_error(std::format("Failed to apply pragma: {}", error));
    }
  }
}
}  // namespace

Database::Database(const std::string& path) {
  int rc = sqlite3_open(path.c_str(), &conn);
  if (rc != SQLITE_OK) {
    std::string error = conn ? sqlite3_errmsg(conn) : "Unknown error";
    if (conn) {
      sqlite3_close(conn);
      conn = nullptr;
    }
    throw std::runtime_error(
        std::format("Failed to open database '{}': {}", path, error));
  }

  // Apply SQLite pragmas for performance and safety
  try {
    apply_pragmas(conn);
  } catch (...) {
    sqlite3_close(conn);
    conn = nullptr;
    throw;
  }

  // TEMPORARY: Set global dbconn for backward compatibility
  // TODO: Remove this in Phase 4 when all code uses Database class
  dbconn = conn;
}

Database::~Database() {
  if (conn) {
    // TEMPORARY: Clear global dbconn if it points to our connection
    if (dbconn == conn) {
      dbconn = nullptr;
    }
    sqlite3_close(conn);
    conn = nullptr;
  }
}

Database::Database(Database&& other) noexcept : conn(other.conn) {
  other.conn = nullptr;
}

Database& Database::operator=(Database&& other) noexcept {
  if (this != &other) {
    if (conn) {
      sqlite3_close(conn);
    }
    conn = other.conn;
    other.conn = nullptr;
  }
  return *this;
}

void Database::begin_transaction() {
  if (!conn) {
    throw std::runtime_error("Database not open");
  }

  char* errmsg = nullptr;
  int rc = sqlite3_exec(conn, "BEGIN TRANSACTION", nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    std::string error = errmsg ? errmsg : "Unknown error";
    sqlite3_free(errmsg);
    throw std::runtime_error(std::format("Failed to begin transaction: {}", error));
  }
}

void Database::commit() {
  if (!conn) {
    throw std::runtime_error("Database not open");
  }

  char* errmsg = nullptr;
  int rc = sqlite3_exec(conn, "COMMIT", nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    std::string error = errmsg ? errmsg : "Unknown error";
    sqlite3_free(errmsg);
    throw std::runtime_error(std::format("Failed to commit transaction: {}", error));
  }
}

void Database::rollback() {
  if (!conn) {
    throw std::runtime_error("Database not open");
  }

  char* errmsg = nullptr;
  int rc = sqlite3_exec(conn, "ROLLBACK", nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    std::string error = errmsg ? errmsg : "Unknown error";
    sqlite3_free(errmsg);
    throw std::runtime_error(std::format("Failed to rollback transaction: {}", error));
  }
}
