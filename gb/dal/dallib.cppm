// SPDX-License-Identifier: Apache-2.0

module;

#include <sqlite3.h>
#undef stdout
#undef stdin
#undef stderr

export module dallib;

export import types;
import std;

export class SqliteError : public std::runtime_error {
  int code_{0};

public:
  explicit SqliteError(const std::string& msg, int code = 0)
      : std::runtime_error(msg), code_(code) {}

  int code() const noexcept {
    return code_;
  }
};

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

  // Run lightweight SQLite maintenance
  void optimize();

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
                              std::int64_t timestamp);
  std::vector<std::tuple<int, int, std::string, std::int64_t>>
  news_get_since(int type, int since_id);
  int news_get_latest_id(int type);
  bool news_purge_type(int type);
  bool news_purge_all();

  // Telegram operations - SQL queries encapsulated in DAL
  std::optional<int> telegram_add(player_t player, governor_t governor,
                                  const std::string& message,
                                  std::int64_t timestamp);
  std::vector<std::tuple<int, int, int, std::string, std::int64_t>>
  telegram_get(player_t player, governor_t governor);
  bool telegram_delete_for_governor(player_t player, governor_t governor);
  int telegram_count(player_t player, governor_t governor);
  bool telegram_purge_all();

  // Planet queries
  int count_non_asteroid_planets();
};

// News item structure (minimal POD for data transfer)
// Note: type is stored as int to avoid circular module dependencies
// Repositories can cast it to/from NewsType enum
export struct NewsItem {
  int id{0};
  int type{0};  // NewsType as int
  std::string message;
  std::int64_t timestamp{0};
};

export struct KeyValue {
  std::variant<std::uint32_t, std::int32_t, std::uint64_t, std::int64_t, double,
               std::string>
      val;

  template <typename T>
    requires std::integral<T>
  constexpr KeyValue(T v) : val(v) {}

  template <FixedString Tag, typename T>
  constexpr KeyValue(ID<Tag, T> id) : val(id.value) {}

  constexpr KeyValue(double v) : val(v) {}
  KeyValue(std::string v) : val(std::move(v)) {}
  KeyValue(const char* v) : val(std::string(v)) {}
};

export class JsonStore {
  Database& db;

public:
  explicit JsonStore(Database& database);

  // Generic CRUD operations
  bool store(const std::string& table, KeyValue id, const std::string& json);
  std::optional<std::string> retrieve(const std::string& table, KeyValue id);
  bool remove(const std::string& table, KeyValue id);

  // ID management
  std::vector<int> list_ids(const std::string& table);
  int find_next_available_id(const std::string& table);

  // Multi-key operations (for Sector, Planet with composite keys)
  bool store_multi(const std::string& table,
                   const std::vector<std::pair<std::string, KeyValue>>& keys,
                   const std::string& json);
  std::optional<std::string>
  retrieve_multi(const std::string& table,
                 const std::vector<std::pair<std::string, KeyValue>>& keys);
};

// Schema initialization
export void initialize_schema(Database& db);
