// SPDX-License-Identifier: Apache-2.0

module;

#include <sqlite3.h>

import std;
#undef stdout

module dallib;

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
    throw std::runtime_error(std::format("{}: {}", action_name, error));
  }
}

SqliteStmtPtr prepare_stmt(sqlite3* conn, const char* sql) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return nullptr;
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
}

Database::~Database() {
  if (conn) {
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
  exec_sql(conn, "BEGIN TRANSACTION", "Failed to begin transaction");
}

void Database::commit() {
  if (!conn) {
    throw std::runtime_error("Database not open");
  }
  exec_sql(conn, "COMMIT", "Failed to commit transaction");
}

void Database::rollback() {
  if (!conn) {
    throw std::runtime_error("Database not open");
  }
  exec_sql(conn, "ROLLBACK", "Failed to rollback transaction");
}

void Database::optimize() {
  if (!conn) {
    throw std::runtime_error("Database not open");
  }
  exec_sql(conn, "PRAGMA optimize;", "Failed to optimize database");
}

// News operations implementation
std::optional<int> Database::news_add(int type, const std::string& message,
                                      std::int64_t timestamp) {
  if (!conn) return std::nullopt;

  const char* sql = R"(
    INSERT INTO tbl_news (type, message, timestamp)
    VALUES (?, ?, ?)
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);
  if (!stmt) {
    return std::nullopt;
  }

  sqlite3_bind_int(stmt.get(), 1, type);
  sqlite3_bind_text(stmt.get(), 2, message.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 3, timestamp);

  int result = sqlite3_step(stmt.get());
  if (result != SQLITE_DONE) {
    return std::nullopt;
  }

  return static_cast<int>(sqlite3_last_insert_rowid(conn));
}

std::vector<std::tuple<int, int, std::string, std::int64_t>>
Database::news_get_since(int type, int since_id) {
  std::vector<std::tuple<int, int, std::string, std::int64_t>> items;
  if (!conn) return items;

  const char* sql = R"(
    SELECT id, type, message, timestamp
    FROM tbl_news
    WHERE type = ? AND id > ?
    ORDER BY timestamp ASC, id ASC
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);
  if (!stmt) {
    return items;
  }

  sqlite3_bind_int(stmt.get(), 1, type);
  sqlite3_bind_int(stmt.get(), 2, since_id);

  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    int id = sqlite3_column_int(stmt.get(), 0);
    int news_type = sqlite3_column_int(stmt.get(), 1);
    const char* msg_text =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
    std::string message = msg_text ? msg_text : "";
    std::int64_t ts = sqlite3_column_int64(stmt.get(), 3);
    items.emplace_back(id, news_type, std::move(message), ts);
  }

  return items;
}

int Database::news_get_latest_id(int type) {
  if (!conn) return 0;

  const char* sql = R"(
    SELECT MAX(id) FROM tbl_news WHERE type = ?
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);
  if (!stmt) {
    return 0;
  }

  sqlite3_bind_int(stmt.get(), 1, type);

  int latest_id = 0;
  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    latest_id = sqlite3_column_int(stmt.get(), 0);
  }

  return latest_id;
}

bool Database::news_purge_type(int type) {
  if (!conn) return false;

  const char* sql = "DELETE FROM tbl_news WHERE type = ?";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);
  if (!stmt) {
    return false;
  }

  sqlite3_bind_int(stmt.get(), 1, type);

  int result = sqlite3_step(stmt.get());
  return result == SQLITE_DONE;
}

bool Database::news_purge_all() {
  if (!conn) return false;

  const char* sql = "DELETE FROM tbl_news";
  char* raw_err = nullptr;
  int result = sqlite3_exec(conn, sql, nullptr, nullptr, &raw_err);
  SqliteErrorPtr err_msg(raw_err);
  return result == SQLITE_OK;
}

// Telegram operations implementation
std::optional<int> Database::telegram_add(player_t player, governor_t governor,
                                          const std::string& message,
                                          std::int64_t timestamp) {
  if (!conn) return std::nullopt;

  const char* sql = R"(
    INSERT INTO tbl_telegram (recipient_player, recipient_governor, message, timestamp)
    VALUES (?, ?, ?, ?)
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);
  if (!stmt) {
    return std::nullopt;
  }

  sqlite3_bind_int(stmt.get(), 1, player.value);
  sqlite3_bind_int(stmt.get(), 2, governor.value);
  sqlite3_bind_text(stmt.get(), 3, message.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 4, timestamp);

  int result = sqlite3_step(stmt.get());
  if (result != SQLITE_DONE) {
    return std::nullopt;
  }

  return static_cast<int>(sqlite3_last_insert_rowid(conn));
}

std::vector<std::tuple<int, int, int, std::string, std::int64_t>>
Database::telegram_get(player_t player, governor_t governor) {
  std::vector<std::tuple<int, int, int, std::string, std::int64_t>> items;
  if (!conn) return items;

  const char* sql = R"(
    SELECT id, recipient_player, recipient_governor, message, timestamp
    FROM tbl_telegram
    WHERE recipient_player = ? AND recipient_governor = ?
    ORDER BY timestamp ASC, id ASC
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);
  if (!stmt) {
    return items;
  }

  sqlite3_bind_int(stmt.get(), 1, player.value);
  sqlite3_bind_int(stmt.get(), 2, governor.value);

  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    int id = sqlite3_column_int(stmt.get(), 0);
    int recv_player = sqlite3_column_int(stmt.get(), 1);
    governor_t recv_governor{sqlite3_column_int(stmt.get(), 2)};
    const char* msg_text =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 3));
    std::string message = msg_text ? msg_text : "";
    std::int64_t ts = sqlite3_column_int64(stmt.get(), 4);
    items.emplace_back(id, recv_player, recv_governor, std::move(message), ts);
  }

  return items;
}

bool Database::telegram_delete_for_governor(player_t player,
                                            governor_t governor) {
  if (!conn) return false;

  const char* sql = R"(
    DELETE FROM tbl_telegram
    WHERE recipient_player = ? AND recipient_governor = ?
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);
  if (!stmt) {
    return false;
  }

  sqlite3_bind_int(stmt.get(), 1, player.value);
  sqlite3_bind_int(stmt.get(), 2, governor.value);

  int result = sqlite3_step(stmt.get());
  return result == SQLITE_DONE;
}

int Database::telegram_count(player_t player, governor_t governor) {
  if (!conn) return 0;

  const char* sql = R"(
    SELECT COUNT(*) FROM tbl_telegram
    WHERE recipient_player = ? AND recipient_governor = ?
  )";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);
  if (!stmt) {
    return 0;
  }

  sqlite3_bind_int(stmt.get(), 1, player.value);
  sqlite3_bind_int(stmt.get(), 2, governor.value);

  int count = 0;
  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt.get(), 0);
  }

  return count;
}

bool Database::telegram_purge_all() {
  if (!conn) return false;

  const char* sql = "DELETE FROM tbl_telegram";
  char* raw_err = nullptr;
  int result = sqlite3_exec(conn, sql, nullptr, nullptr, &raw_err);
  SqliteErrorPtr err_msg(raw_err);
  return result == SQLITE_OK;
}

int Database::count_non_asteroid_planets() {
  if (!conn) return 0;

  const char* sql = "SELECT COUNT(*) FROM tbl_planet WHERE "
                    "json_extract(data, '$.type') != 'ASTEROID'";

  SqliteStmtPtr stmt = prepare_stmt(conn, sql);
  if (!stmt) {
    return 0;
  }

  int count = 0;
  if (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt.get(), 0);
  }

  return count;
}
