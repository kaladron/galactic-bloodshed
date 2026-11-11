// SPDX-License-Identifier: Apache-2.0

/// \file database.cc
/// \brief SQLite3 database connection and transaction management.

module;

#include <sqlite3.h>

import std;

module dallib;

import :impl;

namespace {
struct SqliteDeleter {
  void operator()(char* ptr) const {
    sqlite3_free(ptr);
  }
  void operator()(sqlite3_stmt* stmt) const {
    sqlite3_finalize(stmt);
  }
};

using SqliteErrorPtr = std::unique_ptr<char, SqliteDeleter>;
using SqliteStmtPtr = std::unique_ptr<sqlite3_stmt, SqliteDeleter>;

void exec_sql(sqlite3* db, const char* sql, const char* action_name) {
  char* raw_errmsg = nullptr;
  int rc = sqlite3_exec(db, sql, nullptr, nullptr, &raw_errmsg);
  SqliteErrorPtr errmsg(raw_errmsg);
  if (rc != SQLITE_OK) {
    std::string error = errmsg ? errmsg.get() : "Unknown error";
    throw SqliteError(std::format("{}: {}", action_name, error), rc);
  }
}

SqliteStmtPtr prepare_stmt(sqlite3* conn, const char* sql) {
  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    throw SqliteError(
        std::format("SQLite prepare error: {}", sqlite3_errmsg(conn)), rc);
  }
  return SqliteStmtPtr(stmt);
}

// Apply SQLite pragmas for strict mode (from existing apply_sqlite_strict_mode)
void apply_pragmas(sqlite3* db) {
  const char* pragmas[] = {
      "PRAGMA foreign_keys = ON;",       "PRAGMA journal_mode = WAL;",
      "PRAGMA synchronous = NORMAL;",    "PRAGMA temp_store = MEMORY;",
      "PRAGMA mmap_size = 30000000000;", "PRAGMA page_size = 4096;",
      "PRAGMA cache_size = -64000;",  // 64MB cache
  };

  for (const char* pragma : pragmas) {
    exec_sql(db, pragma, "Failed to apply pragma");
  }
}
}  // namespace

Database::Database(const std::string& path)
    : impl_(std::make_unique<DatabaseImpl>()) {
  int rc = sqlite3_open(path.c_str(), &impl_->conn);
  if (rc != SQLITE_OK) {
    std::string error =
        impl_->conn ? sqlite3_errmsg(impl_->conn) : "Unknown error";
    impl_.reset();
    throw SqliteError(
        std::format("Failed to open database '{}': {}", path, error), rc);
  }

  // Apply SQLite pragmas for performance and safety
  try {
    apply_pragmas(impl_->conn);
  } catch (...) {
    impl_.reset();
    throw;
  }
}

Database::~Database() = default;

Database::Database(Database&& other) noexcept = default;

Database& Database::operator=(Database&& other) noexcept = default;

bool Database::is_open() const noexcept {
  return impl_ && impl_->conn != nullptr;
}

Database::ReadOnlyScope::ReadOnlyScope(Database& db) noexcept : db_(&db) {
  if (db_->impl_) {
    ++db_->impl_->read_only_depth;
  }
}

Database::ReadOnlyScope::~ReadOnlyScope() noexcept {
  if (db_ && db_->impl_) {
    --db_->impl_->read_only_depth;
  }
}

Database::ReadOnlyScope::ReadOnlyScope(ReadOnlyScope&& other) noexcept
    : db_(std::exchange(other.db_, nullptr)) {}

bool Database::is_read_only() const noexcept {
  return impl_ && impl_->read_only_depth > 0;
}

std::int64_t Database::total_changes() const noexcept {
  if (!is_open()) {
    return 0;
  }
  return sqlite3_total_changes64(impl_->conn);
}

void Database::execute_sql(const char* sql, const char* action_name) {
  if (!is_open()) {
    throw SqliteError("Database connection is not open");
  }
  exec_sql(impl_->conn, sql, action_name);
}

void Database::begin_transaction() {
  if (!is_open()) {
    throw SqliteError("Database not open");
  }
  if (is_in_transaction()) {
    throw SqliteError(
        "Transaction already active (transactions are not reentrant)");
  }
  exec_sql(impl_->conn, "BEGIN TRANSACTION", "Failed to begin transaction");
}

void Database::commit() {
  if (!is_open()) {
    throw SqliteError("Database not open");
  }
  exec_sql(impl_->conn, "COMMIT", "Failed to commit transaction");
}

void Database::rollback() {
  if (!is_open()) {
    throw SqliteError("Database not open");
  }
  exec_sql(impl_->conn, "ROLLBACK", "Failed to rollback transaction");
}

bool Database::is_in_transaction() const {
  if (!is_open()) {
    return false;
  }
  return sqlite3_get_autocommit(impl_->conn) == 0;
}

void Database::optimize() {
  if (!is_open()) {
    throw SqliteError("Database not open");
  }
  if (is_read_only()) {
    throw ReadOnlyError("ReadOnlyScope blocked Database::optimize");
  }
  exec_sql(impl_->conn, "PRAGMA optimize;", "Failed to optimize database");
}

void Database::copy_to(Database& dst) const {
  if (!is_open() || !dst.is_open()) {
    throw SqliteError("Database connection is not open");
  }
  if (dst.is_read_only()) {
    throw ReadOnlyError("ReadOnlyScope blocked Database::copy_to");
  }
  if (is_in_transaction() || dst.is_in_transaction()) {
    throw SqliteError("Cannot copy database while a transaction is active");
  }
  if (this == &dst) {
    return;
  }

  sqlite3_backup* backup =
      sqlite3_backup_init(dst.impl_->conn, "main", impl_->conn, "main");
  if (!backup) {
    throw SqliteError(std::format("Failed to init SQLite backup: {}",
                                  sqlite3_errmsg(dst.impl_->conn)),
                      sqlite3_errcode(dst.impl_->conn));
  }
  const int step_rc = sqlite3_backup_step(backup, -1);
  const int finish_rc = sqlite3_backup_finish(backup);
  if (step_rc != SQLITE_DONE || finish_rc != SQLITE_OK) {
    throw SqliteError(std::format("Failed to copy SQLite database: {}",
                                  sqlite3_errmsg(dst.impl_->conn)),
                      finish_rc != SQLITE_OK ? finish_rc : step_rc);
  }
}

int Database::count_non_asteroid_planets() {
  if (!is_open()) {
    throw SqliteError("Database connection is not open");
  }
  sqlite3* conn = impl_->conn;

  const char* sql = "SELECT COUNT(*) FROM tbl_planet WHERE "
                    "json_extract(data, '$.type') != 1";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);

  int count = 0;
  int step_rc = sqlite3_step(stmt.get());
  if (step_rc == SQLITE_ROW) {
    count = sqlite3_column_int(stmt.get(), 0);
  } else if (step_rc != SQLITE_DONE) {
    throw SqliteError(
        std::format("SQLite step error counting non-asteroid planets: {}",
                    sqlite3_errmsg(conn)),
        step_rc);
  }

  return count;
}
