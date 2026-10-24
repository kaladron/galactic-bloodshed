// SPDX-License-Identifier: Apache-2.0

/// \file dallib.cppm
/// \brief Data Access Layer module interface encapsulating SQLite persistence.

export module dallib;

export import types;
import std;

export class SqliteError : public std::runtime_error {
  int code_{0};

public:
  explicit SqliteError(const std::string& msg, int code = 0)
      : std::runtime_error(msg), code_(code) {}

  [[nodiscard]] int code() const noexcept {
    return code_;
  }
};

struct DatabaseImpl;
export class Database;
export void initialize_schema(Database& db);

export class Database {
  std::unique_ptr<DatabaseImpl> impl_;

  friend class JsonStore;
  friend void initialize_schema(Database& db);

  void execute_sql(const char* sql, const char* action_name);

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
  [[nodiscard]] bool is_in_transaction() const;

  // Run lightweight SQLite maintenance
  void optimize();

  // Check if database is open
  [[nodiscard]] bool is_open() const noexcept;

  // Planet queries
  int count_non_asteroid_planets();
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
  bool store(const std::string& table, const KeyValue& id,
             const std::string& json);
  std::optional<std::string> retrieve(const std::string& table,
                                      const KeyValue& id);
  bool remove(const std::string& table, const KeyValue& id);

  // ID management
  std::vector<int> list_ids(const std::string& table);
  std::int64_t find_next_available_id(const std::string& table);

  // Query operations
  std::vector<int> query_ids(const std::string& table,
                             const std::string& where_clause,
                             const std::vector<KeyValue>& params = {});
  std::vector<std::string>
  retrieve_where(const std::string& table, const std::string& where_clause = "",
                 const std::vector<KeyValue>& params = {});
  bool remove_where(const std::string& table,
                    const std::string& where_clause = "",
                    const std::vector<KeyValue>& params = {});
  int count_where(const std::string& table,
                  const std::string& where_clause = "",
                  const std::vector<KeyValue>& params = {});

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
