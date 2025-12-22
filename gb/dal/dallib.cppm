// SPDX-License-Identifier: Apache-2.0

module;

#include <sqlite3.h>

export module dallib;

import std.compat;

export class Database {
  sqlite3* conn = nullptr;

public:
  // Constructor: opens database connection
  // path defaults to ":memory:" for in-memory database
  explicit Database(const std::string& path = ":memory:");

  // Destructor: closes connection
  ~Database();

  // Delete copy, allow move
  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;
  Database(Database&&) noexcept;
  Database& operator=(Database&&) noexcept;

  // Transaction support
  void begin_transaction();
  void commit();
  void rollback();

  // Check if database is open
  bool is_open() const {
    return conn != nullptr;
  }

  // Internal access for JsonStore only
  // Note: This should only be used by DAL components
  sqlite3* connection() {
    return conn;
  }

  // News operations - SQL queries encapsulated in DAL
  std::optional<int> news_add(int type, const std::string& message,
                              int64_t timestamp);
  std::vector<std::tuple<int, int, std::string, int64_t>>
  news_get_since(int type, int since_id);
  int news_get_latest_id(int type);
  bool news_purge_type(int type);
  bool news_purge_all();
};

// News item structure (minimal POD for data transfer)
// Note: type is stored as int to avoid circular module dependencies
// Repositories can cast it to/from NewsType enum
export struct NewsItem {
  int id{0};
  int type{0};  // NewsType as int
  std::string message;
  int64_t timestamp{0};
};

export class JsonStore {
  Database& db;

public:
  explicit JsonStore(Database& database);

  // Generic CRUD operations
  bool store(const std::string& table, int id, const std::string& json);
  std::optional<std::string> retrieve(const std::string& table, int id);
  bool remove(const std::string& table, int id);

  // ID management
  std::vector<int> list_ids(const std::string& table);
  int find_next_available_id(const std::string& table);

  // Multi-key operations (for Sector, Planet with composite keys)
  bool store_multi(const std::string& table,
                   const std::vector<std::pair<std::string, int>>& keys,
                   const std::string& json);
  std::optional<std::string>
  retrieve_multi(const std::string& table,
                 const std::vector<std::pair<std::string, int>>& keys);
};

// Schema initialization
export void initialize_schema(Database& db);
