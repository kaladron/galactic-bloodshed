// SPDX-License-Identifier: Apache-2.0

module;

#include <sqlite3.h>

import std;
#undef stdout

module dallib;

namespace {
void bind_key(sqlite3_stmt* stmt, const KeyValue& key, int idx = 1) {
  std::visit(
      [stmt, idx](auto&& v) {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, std::uint32_t> ||
                      std::is_same_v<V, std::int32_t>) {
          sqlite3_bind_int(stmt, idx, static_cast<int>(v));
        } else if constexpr (std::is_same_v<V, std::uint64_t> ||
                             std::is_same_v<V, std::int64_t>) {
          sqlite3_bind_int64(stmt, idx, static_cast<sqlite3_int64>(v));
        } else if constexpr (std::is_same_v<V, double>) {
          sqlite3_bind_double(stmt, idx, v);
        } else if constexpr (std::is_same_v<V, std::string>) {
          sqlite3_bind_text(stmt, idx, v.c_str(), -1, SQLITE_TRANSIENT);
        }
      },
      key.val);
}

void bind_keys(sqlite3_stmt* stmt,
               const std::vector<std::pair<std::string, KeyValue>>& keys) {
  for (std::size_t i = 0; i < keys.size(); ++i) {
    bind_key(stmt, keys[i].second, static_cast<int>(i + 1));
  }
}
}  // namespace

JsonStore::JsonStore(Database& database) : db(database) {}

bool JsonStore::store(const std::string& table, KeyValue id,
                      const std::string& json) {
  if (!db.is_open()) {
    throw SqliteError("Database connection is not open");
  }

  std::string sql =
      std::format("REPLACE INTO {} (id, data) VALUES (?, ?)", table);

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    throw SqliteError(std::format("SQLite prepare error in table '{}': {}",
                                  table, sqlite3_errmsg(db.connection())),
                      rc);
  }

  bind_key(stmt, id);
  sqlite3_bind_text(stmt, 2, json.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE) {
    throw SqliteError(std::format("SQLite step error storing to table '{}': {}",
                                  table, sqlite3_errmsg(db.connection())),
                      rc);
  }

  return true;
}

std::optional<std::string> JsonStore::retrieve(const std::string& table,
                                               KeyValue id) {
  if (!db.is_open()) {
    throw SqliteError("Database connection is not open");
  }

  std::string sql = std::format("SELECT data FROM {} WHERE id = ?", table);

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    throw SqliteError(std::format("SQLite prepare error in table '{}': {}",
                                  table, sqlite3_errmsg(db.connection())),
                      rc);
  }

  bind_key(stmt, id);

  int step_rc = sqlite3_step(stmt);
  std::optional<std::string> result;

  if (step_rc == SQLITE_ROW) {
    const char* data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (data) {
      result = std::string(data);
    }
  } else if (step_rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw SqliteError(std::format("SQLite step error querying table '{}': {}",
                                  table, sqlite3_errmsg(db.connection())),
                      step_rc);
  }

  sqlite3_finalize(stmt);
  return result;
}

bool JsonStore::remove(const std::string& table, KeyValue id) {
  if (!db.is_open()) {
    throw SqliteError("Database connection is not open");
  }

  std::string sql = std::format("DELETE FROM {} WHERE id = ?", table);

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    throw SqliteError(std::format("SQLite prepare error in table '{}': {}",
                                  table, sqlite3_errmsg(db.connection())),
                      rc);
  }

  bind_key(stmt, id);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE) {
    throw SqliteError(
        std::format("SQLite step error deleting from table '{}': {}", table,
                    sqlite3_errmsg(db.connection())),
        rc);
  }

  return true;
}

std::vector<int> JsonStore::list_ids(const std::string& table) {
  if (!db.is_open()) {
    throw SqliteError("Database connection is not open");
  }

  std::vector<int> ids;
  std::string sql = std::format("SELECT id FROM {} ORDER BY id", table);

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    throw SqliteError(std::format("SQLite prepare error in table '{}': {}",
                                  table, sqlite3_errmsg(db.connection())),
                      rc);
  }

  int step_rc;
  while ((step_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    ids.push_back(sqlite3_column_int(stmt, 0));
  }

  if (step_rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw SqliteError(
        std::format("SQLite step error listing IDs in table '{}': {}", table,
                    sqlite3_errmsg(db.connection())),
        step_rc);
  }

  sqlite3_finalize(stmt);
  return ids;
}

int JsonStore::find_next_available_id(const std::string& table) {
  if (!db.is_open()) {
    throw SqliteError("Database connection is not open");
  }

  std::string sql = std::format(R"(
    WITH RECURSIVE cnt(x) AS (
      SELECT 1
      UNION ALL
      SELECT x+1 FROM cnt
      LIMIT (SELECT IFNULL(MAX(id), 0) + 1 FROM {})
    )
    SELECT x FROM cnt
    WHERE x NOT IN (SELECT id FROM {})
    ORDER BY x
    LIMIT 1
  )",
                                table, table);

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    throw SqliteError(std::format("SQLite prepare error in table '{}': {}",
                                  table, sqlite3_errmsg(db.connection())),
                      rc);
  }

  int result = 1;  // Default if no rows exist
  int step_rc = sqlite3_step(stmt);
  if (step_rc == SQLITE_ROW) {
    result = sqlite3_column_int(stmt, 0);
  } else if (step_rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw SqliteError(
        std::format(
            "SQLite step error in find_next_available_id for table '{}': {}",
            table, sqlite3_errmsg(db.connection())),
        step_rc);
  }

  sqlite3_finalize(stmt);
  return result;
}

bool JsonStore::store_multi(
    const std::string& table,
    const std::vector<std::pair<std::string, KeyValue>>& keys,
    const std::string& json) {
  if (!db.is_open()) {
    throw SqliteError("Database connection is not open");
  }
  if (keys.empty()) return false;

  std::string columns;
  std::string placeholders;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (i > 0) {
      columns += ", ";
      placeholders += ", ";
    }
    columns += keys[i].first;
    placeholders += "?";
  }
  columns += ", data";
  placeholders += ", ?";

  std::string sql = std::format("REPLACE INTO {} ({}) VALUES ({})", table,
                                columns, placeholders);

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    throw SqliteError(std::format("SQLite prepare error in table '{}': {}",
                                  table, sqlite3_errmsg(db.connection())),
                      rc);
  }

  bind_keys(stmt, keys);
  sqlite3_bind_text(stmt, static_cast<int>(keys.size() + 1), json.c_str(), -1,
                    SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  if (rc != SQLITE_DONE) {
    throw SqliteError(
        std::format("SQLite step error storing multi to table '{}': {}", table,
                    sqlite3_errmsg(db.connection())),
        rc);
  }

  return true;
}

std::optional<std::string> JsonStore::retrieve_multi(
    const std::string& table,
    const std::vector<std::pair<std::string, KeyValue>>& keys) {
  if (!db.is_open()) {
    throw SqliteError("Database connection is not open");
  }
  if (keys.empty()) return std::nullopt;

  std::string where;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (i > 0) where += " AND ";
    where += std::format("{} = ?", keys[i].first);
  }

  std::string sql = std::format("SELECT data FROM {} WHERE {}", table, where);

  sqlite3_stmt* stmt = nullptr;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    throw SqliteError(std::format("SQLite prepare error in table '{}': {}",
                                  table, sqlite3_errmsg(db.connection())),
                      rc);
  }

  bind_keys(stmt, keys);

  int step_rc = sqlite3_step(stmt);
  std::optional<std::string> result;

  if (step_rc == SQLITE_ROW) {
    const char* data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (data) {
      result = std::string(data);
    }
  } else if (step_rc != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw SqliteError(std::format("SQLite step error querying table '{}': {}",
                                  table, sqlite3_errmsg(db.connection())),
                      step_rc);
  }

  sqlite3_finalize(stmt);
  return result;
}
