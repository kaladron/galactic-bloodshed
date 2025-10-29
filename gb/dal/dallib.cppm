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
  bool is_open() const { return conn != nullptr; }

  // Internal access for JsonStore only
  // Note: This should only be used by DAL components
  sqlite3* connection() { return conn; }
};
