// SPDX-License-Identifier: Apache-2.0

module;

#include <sqlite3.h>

import std;
#undef stdout

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
  exec_sql(impl_->conn, "PRAGMA optimize;", "Failed to optimize database");
}

// News operations implementation
news_id_t Database::news_add(int type, const std::string& message,
                             std::int64_t timestamp) {
  if (!is_open()) {
    throw SqliteError("Database connection is not open");
  }
  sqlite3* conn = impl_->conn;

  const char* sql = R"(
    INSERT INTO tbl_news (type, message, timestamp)
    VALUES (?, ?, ?)
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);

  sqlite3_bind_int(stmt.get(), 1, type);
  sqlite3_bind_text(stmt.get(), 2, message.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 3, timestamp);

  int result = sqlite3_step(stmt.get());
  if (result != SQLITE_DONE) {
    throw SqliteError(std::format("SQLite step error inserting news item: {}",
                                  sqlite3_errmsg(conn)),
                      result);
  }

  return news_id_t{sqlite3_last_insert_rowid(conn)};
}

std::vector<NewsItem> Database::news_get_since(int type, news_id_t since_id) {
  if (!is_open()) {
    throw SqliteError("Database connection is not open");
  }
  sqlite3* conn = impl_->conn;

  std::vector<NewsItem> items;

  const char* sql = R"(
    SELECT id, type, message, timestamp
    FROM tbl_news
    WHERE type = ? AND id > ?
    ORDER BY timestamp ASC, id ASC
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);

  sqlite3_bind_int(stmt.get(), 1, type);
  sqlite3_bind_int64(stmt.get(), 2, since_id.value);

  int step_rc;
  while ((step_rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    const news_id_t id{sqlite3_column_int64(stmt.get(), 0)};
    const int news_type = sqlite3_column_int(stmt.get(), 1);
    const char* msg_text =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
    std::string message = msg_text ? msg_text : "";
    const std::int64_t ts = sqlite3_column_int64(stmt.get(), 3);
    items.push_back(NewsItem{
        .id = id,
        .type = news_type,
        .message = std::move(message),
        .timestamp = ts,
    });
  }

  if (step_rc != SQLITE_DONE) {
    throw SqliteError(std::format("SQLite step error retrieving news items: {}",
                                  sqlite3_errmsg(conn)),
                      step_rc);
  }

  return items;
}

bool Database::news_purge_type(int type) {
  if (!is_open()) {
    throw SqliteError("Database connection is not open");
  }
  sqlite3* conn = impl_->conn;

  const char* sql = "DELETE FROM tbl_news WHERE type = ?";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);

  sqlite3_bind_int(stmt.get(), 1, type);

  int result = sqlite3_step(stmt.get());
  if (result != SQLITE_DONE) {
    throw SqliteError(std::format("SQLite step error purging news type: {}",
                                  sqlite3_errmsg(conn)),
                      result);
  }
  return true;
}

bool Database::news_purge_all() {
  if (!is_open()) {
    throw SqliteError("Database connection is not open");
  }
  sqlite3* conn = impl_->conn;

  const char* sql = "DELETE FROM tbl_news";
  exec_sql(conn, sql, "Failed to purge news");
  return true;
}

// Telegram operations implementation
telegram_id_t Database::telegram_add(player_t player, governor_t governor,
                                     const std::string& message,
                                     std::int64_t timestamp) {
  if (!is_open()) {
    throw SqliteError("Database connection is not open");
  }
  sqlite3* conn = impl_->conn;

  const char* sql = R"(
    INSERT INTO tbl_telegram (recipient_player, recipient_governor, message, timestamp)
    VALUES (?, ?, ?, ?)
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);

  sqlite3_bind_int(stmt.get(), 1, player.value);
  sqlite3_bind_int(stmt.get(), 2, governor.value);
  sqlite3_bind_text(stmt.get(), 3, message.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 4, timestamp);

  int result = sqlite3_step(stmt.get());
  if (result != SQLITE_DONE) {
    throw SqliteError(std::format("SQLite step error inserting telegram: {}",
                                  sqlite3_errmsg(conn)),
                      result);
  }

  return telegram_id_t{sqlite3_last_insert_rowid(conn)};
}

std::vector<TelegramItem> Database::telegram_get(player_t player,
                                                 governor_t governor) {
  if (!is_open()) {
    throw SqliteError("Database connection is not open");
  }
  sqlite3* conn = impl_->conn;

  std::vector<TelegramItem> items;

  const char* sql = R"(
    SELECT id, recipient_player, recipient_governor, message, timestamp
    FROM tbl_telegram
    WHERE recipient_player = ? AND recipient_governor = ?
    ORDER BY timestamp ASC, id ASC
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);

  sqlite3_bind_int(stmt.get(), 1, player.value);
  sqlite3_bind_int(stmt.get(), 2, governor.value);

  int step_rc;
  while ((step_rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
    const telegram_id_t id{sqlite3_column_int64(stmt.get(), 0)};
    const player_t recv_player{sqlite3_column_int(stmt.get(), 1)};
    const governor_t recv_governor{sqlite3_column_int(stmt.get(), 2)};
    const char* msg_text =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
    std::string message = msg_text ? msg_text : "";
    const std::int64_t ts = sqlite3_column_int64(stmt.get(), 4);
    items.push_back(TelegramItem{
        .id = id,
        .recipient_player = recv_player,
        .recipient_governor = recv_governor,
        .message = std::move(message),
        .timestamp = ts,
    });
  }

  if (step_rc != SQLITE_DONE) {
    throw SqliteError(std::format("SQLite step error retrieving telegrams: {}",
                                  sqlite3_errmsg(conn)),
                      step_rc);
  }

  return items;
}

bool Database::telegram_delete_for_governor(player_t player,
                                            governor_t governor) {
  if (!is_open()) {
    throw SqliteError("Database connection is not open");
  }
  sqlite3* conn = impl_->conn;

  const char* sql = R"(
    DELETE FROM tbl_telegram
    WHERE recipient_player = ? AND recipient_governor = ?
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);

  sqlite3_bind_int(stmt.get(), 1, player.value);
  sqlite3_bind_int(stmt.get(), 2, governor.value);

  int result = sqlite3_step(stmt.get());
  if (result != SQLITE_DONE) {
    throw SqliteError(std::format("SQLite step error deleting telegrams: {}",
                                  sqlite3_errmsg(conn)),
                      result);
  }
  return true;
}

int Database::telegram_count(player_t player, governor_t governor) {
  if (!is_open()) {
    throw SqliteError("Database connection is not open");
  }
  sqlite3* conn = impl_->conn;

  const char* sql = R"(
    SELECT COUNT(*) FROM tbl_telegram
    WHERE recipient_player = ? AND recipient_governor = ?
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);

  sqlite3_bind_int(stmt.get(), 1, player.value);
  sqlite3_bind_int(stmt.get(), 2, governor.value);

  int count = 0;
  int step_rc = sqlite3_step(stmt.get());
  if (step_rc == SQLITE_ROW) {
    count = sqlite3_column_int(stmt.get(), 0);
  } else if (step_rc != SQLITE_DONE) {
    throw SqliteError(std::format("SQLite step error counting telegrams: {}",
                                  sqlite3_errmsg(conn)),
                      step_rc);
  }

  return count;
}

bool Database::telegram_purge_all() {
  if (!is_open()) {
    throw SqliteError("Database connection is not open");
  }
  sqlite3* conn = impl_->conn;

  const char* sql = "DELETE FROM tbl_telegram";
  exec_sql(conn, sql, "Failed to purge telegrams");
  return true;
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
