// SPDX-License-Identifier: Apache-2.0

module;

#include <sqlite3.h>

import std;
#undef stdout

module dallib;

JsonStore::JsonStore(Database& database) : db(database) {}

bool JsonStore::store(const std::string& table, KeyValue id,
                      const std::string& json) {
  if (!db.is_open()) return false;

  std::string sql =
      std::format("REPLACE INTO {} (id, data) VALUES (?, ?)", table);

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return false;

  std::visit(
      [stmt, idx = 1](auto&& v) {
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
      id.val);
  sqlite3_bind_text(stmt, 2, json.c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return rc == SQLITE_DONE;
}

std::optional<std::string> JsonStore::retrieve(const std::string& table,
                                               KeyValue id) {
  if (!db.is_open()) return std::nullopt;

  std::string sql = std::format("SELECT data FROM {} WHERE id = ?", table);

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return std::nullopt;

  std::visit(
      [stmt, idx = 1](auto&& v) {
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
      id.val);

  std::optional<std::string> result;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (data) {
      result = std::string(data);
    }
  }

  sqlite3_finalize(stmt);
  return result;
}

bool JsonStore::remove(const std::string& table, KeyValue id) {
  if (!db.is_open()) return false;

  std::string sql = std::format("DELETE FROM {} WHERE id = ?", table);

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return false;

  std::visit(
      [stmt, idx = 1](auto&& v) {
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
      id.val);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return rc == SQLITE_DONE;
}

std::vector<int> JsonStore::list_ids(const std::string& table) {
  std::vector<int> ids;
  if (!db.is_open()) return ids;

  std::string sql = std::format("SELECT id FROM {} ORDER BY id", table);

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return ids;

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    ids.push_back(sqlite3_column_int(stmt, 0));
  }

  sqlite3_finalize(stmt);
  return ids;
}

int JsonStore::find_next_available_id(const std::string& table) {
  if (!db.is_open()) return 1;

  // Find the first gap in IDs using recursive CTE, or return 1 if table is
  // empty
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

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return 1;

  int result = 1;  // Default if no rows exist
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    result = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return result;
}

bool JsonStore::store_multi(
    const std::string& table,
    const std::vector<std::pair<std::string, KeyValue>>& keys,
    const std::string& json) {
  if (!db.is_open() || keys.empty()) return false;

  // Build REPLACE INTO statement with named columns
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

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return false;

  // Bind key values
  for (std::size_t i = 0; i < keys.size(); ++i) {
    std::visit(
        [stmt, idx = static_cast<int>(i + 1)](auto&& v) {
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
        keys[i].second.val);
  }
  // Bind JSON data
  sqlite3_bind_text(stmt, static_cast<int>(keys.size() + 1), json.c_str(), -1,
                    SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return rc == SQLITE_DONE;
}

std::optional<std::string> JsonStore::retrieve_multi(
    const std::string& table,
    const std::vector<std::pair<std::string, KeyValue>>& keys) {
  if (!db.is_open() || keys.empty()) return std::nullopt;

  // Build WHERE clause with all keys
  std::string where;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    if (i > 0) where += " AND ";
    where += std::format("{} = ?", keys[i].first);
  }

  std::string sql = std::format("SELECT data FROM {} WHERE {}", table, where);

  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db.connection(), sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) return std::nullopt;

  // Bind key values
  for (std::size_t i = 0; i < keys.size(); ++i) {
    std::visit(
        [stmt, idx = static_cast<int>(i + 1)](auto&& v) {
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
        keys[i].second.val);
  }

  std::optional<std::string> result;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* data =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (data) {
      result = std::string(data);
    }
  }

  sqlite3_finalize(stmt);
  return result;
}
