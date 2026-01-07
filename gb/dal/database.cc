// SPDX-License-Identifier: Apache-2.0

module;

import std.compat;

#include <sqlite3.h>

module dallib;

namespace {
// Apply SQLite pragmas for strict mode (from existing apply_sqlite_strict_mode)
void apply_pragmas(sqlite3* db) {
  const char* pragmas[] = {
      "PRAGMA foreign_keys = ON;",       "PRAGMA journal_mode = WAL;",
      "PRAGMA synchronous = NORMAL;",    "PRAGMA temp_store = MEMORY;",
      "PRAGMA mmap_size = 30000000000;", "PRAGMA page_size = 4096;",
      "PRAGMA cache_size = -64000;",  // 64MB cache
  };

  for (const char* pragma : pragmas) {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db, pragma, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
      std::string error = errmsg ? errmsg : "Unknown error";
      sqlite3_free(errmsg);
      throw std::runtime_error(
          std::format("Failed to apply pragma: {}", error));
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

  char* errmsg = nullptr;
  int rc = sqlite3_exec(conn, "BEGIN TRANSACTION", nullptr, nullptr, &errmsg);
  if (rc != SQLITE_OK) {
    std::string error = errmsg ? errmsg : "Unknown error";
    sqlite3_free(errmsg);
    throw std::runtime_error(
        std::format("Failed to begin transaction: {}", error));
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
    throw std::runtime_error(
        std::format("Failed to commit transaction: {}", error));
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
    throw std::runtime_error(
        std::format("Failed to rollback transaction: {}", error));
  }
}

// News operations implementation
std::optional<int> Database::news_add(int type, const std::string& message,
                                      int64_t timestamp) {
  if (!conn) return std::nullopt;

  const char* sql = R"(
    INSERT INTO tbl_news (type, message, timestamp)
    VALUES (?, ?, ?)
  )";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return std::nullopt;
  }

  sqlite3_bind_int(stmt, 1, type);
  sqlite3_bind_text(stmt, 2, message.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 3, timestamp);

  int result = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (result != SQLITE_DONE) {
    return std::nullopt;
  }

  return static_cast<int>(sqlite3_last_insert_rowid(conn));
}

std::vector<std::tuple<int, int, std::string, int64_t>>
Database::news_get_since(int type, int since_id) {
  std::vector<std::tuple<int, int, std::string, int64_t>> items;
  if (!conn) return items;

  const char* sql = R"(
    SELECT id, type, message, timestamp
    FROM tbl_news
    WHERE type = ? AND id > ?
    ORDER BY timestamp ASC, id ASC
  )";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return items;
  }

  sqlite3_bind_int(stmt, 1, type);
  sqlite3_bind_int(stmt, 2, since_id);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int id = sqlite3_column_int(stmt, 0);
    int news_type = sqlite3_column_int(stmt, 1);
    const char* msg_text =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    std::string message = msg_text ? msg_text : "";
    int64_t ts = sqlite3_column_int64(stmt, 3);
    items.emplace_back(id, news_type, std::move(message), ts);
  }

  sqlite3_finalize(stmt);
  return items;
}

int Database::news_get_latest_id(int type) {
  if (!conn) return 0;

  const char* sql = R"(
    SELECT MAX(id) FROM tbl_news WHERE type = ?
  )";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return 0;
  }

  sqlite3_bind_int(stmt, 1, type);

  int latest_id = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    latest_id = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return latest_id;
}

bool Database::news_purge_type(int type) {
  if (!conn) return false;

  const char* sql = "DELETE FROM tbl_news WHERE type = ?";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_int(stmt, 1, type);

  int result = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return result == SQLITE_DONE;
}

bool Database::news_purge_all() {
  if (!conn) return false;

  const char* sql = "DELETE FROM tbl_news";
  char* err_msg = nullptr;
  int result = sqlite3_exec(conn, sql, nullptr, nullptr, &err_msg);
  if (err_msg) {
    sqlite3_free(err_msg);
  }
  return result == SQLITE_OK;
}

// Telegram operations implementation
std::optional<int> Database::telegram_add(player_t player, governor_t governor,
                                          const std::string& message,
                                          int64_t timestamp) {
  if (!conn) return std::nullopt;

  const char* sql = R"(
    INSERT INTO tbl_telegram (recipient_player, recipient_governor, message, timestamp)
    VALUES (?, ?, ?, ?)
  )";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return std::nullopt;
  }

  sqlite3_bind_int(stmt, 1, player);
  sqlite3_bind_int(stmt, 2, governor);
  sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 4, timestamp);

  int result = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (result != SQLITE_DONE) {
    return std::nullopt;
  }

  return static_cast<int>(sqlite3_last_insert_rowid(conn));
}

std::vector<std::tuple<int, int, int, std::string, int64_t>>
Database::telegram_get(player_t player, governor_t governor) {
  std::vector<std::tuple<int, int, int, std::string, int64_t>> items;
  if (!conn) return items;

  const char* sql = R"(
    SELECT id, recipient_player, recipient_governor, message, timestamp
    FROM tbl_telegram
    WHERE recipient_player = ? AND recipient_governor = ?
    ORDER BY timestamp ASC, id ASC
  )";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return items;
  }

  sqlite3_bind_int(stmt, 1, player);
  sqlite3_bind_int(stmt, 2, governor);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int id = sqlite3_column_int(stmt, 0);
    int recv_player = sqlite3_column_int(stmt, 1);
    int recv_governor = sqlite3_column_int(stmt, 2);
    const char* msg_text =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    std::string message = msg_text ? msg_text : "";
    int64_t ts = sqlite3_column_int64(stmt, 4);
    items.emplace_back(id, recv_player, recv_governor, std::move(message), ts);
  }

  sqlite3_finalize(stmt);
  return items;
}

bool Database::telegram_delete_for_governor(player_t player,
                                            governor_t governor) {
  if (!conn) return false;

  const char* sql = R"(
    DELETE FROM tbl_telegram
    WHERE recipient_player = ? AND recipient_governor = ?
  )";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }

  sqlite3_bind_int(stmt, 1, player);
  sqlite3_bind_int(stmt, 2, governor);

  int result = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return result == SQLITE_DONE;
}

int Database::telegram_count(player_t player, governor_t governor) {
  if (!conn) return 0;

  const char* sql = R"(
    SELECT COUNT(*) FROM tbl_telegram
    WHERE recipient_player = ? AND recipient_governor = ?
  )";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return 0;
  }

  sqlite3_bind_int(stmt, 1, player);
  sqlite3_bind_int(stmt, 2, governor);

  int count = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return count;
}

bool Database::telegram_purge_all() {
  if (!conn) return false;

  const char* sql = "DELETE FROM tbl_telegram";
  char* err_msg = nullptr;
  int result = sqlite3_exec(conn, sql, nullptr, nullptr, &err_msg);
  if (err_msg) {
    sqlite3_free(err_msg);
  }
  return result == SQLITE_OK;
}

int Database::count_non_asteroid_planets() {
  if (!conn) return 0;

  const char* sql = "SELECT COUNT(*) FROM tbl_planet WHERE "
                    "json_extract(data, '$.type') != 'ASTEROID'";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    return 0;
  }

  int count = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return count;
}
