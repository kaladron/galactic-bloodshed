// SPDX-License-Identifier: Apache-2.0

/// \file repositories.cppm
/// \brief Primary module interface for data access repositories
/// (gb.repositories).

export module gb.repositories;

import dallib;
import gb.entities;
import std;

// Base template for repositories
// Provides common CRUD operations for entity types
// Derived classes must implement serialize/deserialize for their specific type
export template <typename T>
class Repository {
protected:
  JsonStore& store;
  std::string table_name;

  // Derived classes must implement these for their specific type
  [[nodiscard]] virtual std::optional<std::string>
  serialize(const T& entity) const = 0;
  [[nodiscard]] virtual std::optional<T>
  deserialize(const std::string& json) const = 0;

public:
  Repository(JsonStore& js, std::string table)
      : store(js), table_name(std::move(table)) {}

  virtual ~Repository() = default;

  // Delete copy, allow move
  Repository(const Repository&) = delete;
  Repository& operator=(const Repository&) = delete;
  Repository(Repository&&) = default;
  Repository& operator=(Repository&&) = default;

  // Save entity with given ID
  bool save(const KeyValue& id, const T& entity) {
    if (auto json = serialize(entity)) {
      return store.store(table_name, id, *json);
    }
    return false;
  }

  // Find entity by ID
  std::optional<T> find(const KeyValue& id) {
    return store.retrieve(table_name, id).and_then([this](const auto& json) {
      return deserialize(json);
    });
  }

  // Find entity by composite keys
  std::optional<T>
  find_multi(const std::vector<std::pair<std::string, KeyValue>>& keys) {
    return store.retrieve_multi(table_name, keys)
        .and_then([this](const auto& json) { return deserialize(json); });
  }

  // Remove entity by ID
  bool remove(const KeyValue& id) {
    return store.remove(table_name, id);
  }

  // Get next available ID
  int next_available_id() {
    return store.find_next_available_id(table_name);
  }

  // List all IDs in the table
  std::vector<int> list_ids() {
    return store.list_ids(table_name);
  }
};

// RaceRepository - provides type-safe access to Race entities
export class RaceRepository : public Repository<Race> {
public:
  RaceRepository(JsonStore& store);

  // Domain-specific methods
  std::optional<Race> find_by_player(player_t player);
  bool save(const Race& race);

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Race& race) const override;
  [[nodiscard]] std::optional<Race>
  deserialize(const std::string& json_str) const override;
};

// ShipRepository - provides type-safe access to Ship entities
export class ShipRepository : public Repository<Ship> {
public:
  ShipRepository(JsonStore& store);

  // Domain-specific methods
  std::optional<Ship> find_by_number(shipnum_t num);
  [[nodiscard]] std::unique_ptr<Ship> find_ship(shipnum_t num);
  bool save(const Ship& ship);
  void delete_ship(shipnum_t num);
  shipnum_t next_ship_number();
  shipnum_t count_all_ships();

  // Spatial and indexed query methods
  [[nodiscard]] std::vector<shipnum_t> find_in_star_system(starnum_t star_id);
  [[nodiscard]] std::vector<shipnum_t> find_in_star(starnum_t star_id);
  [[nodiscard]] std::vector<shipnum_t> find_on_planet(starnum_t star_id,
                                                      planetnum_t planet_id);
  [[nodiscard]] std::vector<shipnum_t> find_in_hangar(shipnum_t carrier_id);
  [[nodiscard]] std::vector<shipnum_t> find_by_owner(player_t owner_id);
  [[nodiscard]] std::vector<shipnum_t> find_at_scope(ScopeLevel scope);
  [[nodiscard]] std::vector<shipnum_t> find_alive();

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Ship& ship) const override;
  [[nodiscard]] std::optional<Ship>
  deserialize(const std::string& json_str) const override;
};

// PlanetRepository - provides type-safe access to Planet entities
// Planets are stored with composite key (star_id, planet_order)
export class PlanetRepository : public Repository<Planet> {
public:
  PlanetRepository(JsonStore& store);

  // Domain-specific methods
  // Note: Planets use composite keys (star_id, planet_order) in database
  std::optional<Planet> find_by_location(starnum_t star, planetnum_t pnum);
  bool save(const Planet& planet);

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Planet& planet) const override;
  [[nodiscard]] std::optional<Planet>
  deserialize(const std::string& json_str) const override;

private:
  // Helper for internal use with explicit parameters
  bool save_planet_impl(const Planet& planet, starnum_t star, planetnum_t pnum);
};

// StarRepository - provides type-safe access to Star entities
export class StarRepository : public Repository<Star> {
public:
  StarRepository(JsonStore& store);

  // Domain-specific methods
  std::optional<Star> find_by_number(starnum_t num);
  bool save(const Star& star);

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Star& star) const override;
  [[nodiscard]] std::optional<Star>
  deserialize(const std::string& json_str) const override;
};

// SectorRepository - provides type-safe access to Sector entities
// Note: Sectors use composite keys (star_id, planet_order, xpos, ypos) in
// database
export class SectorRepository : public Repository<Sector> {
public:
  SectorRepository(JsonStore& store);

  // Domain-specific methods working with sector_struct and Coordinates
  [[nodiscard]] sector_struct load(starnum_t star_id, planetnum_t planet_order,
                                   Coordinates coords);
  [[nodiscard]] sector_struct load(starnum_t star_id, planetnum_t planet_order,
                                   std::size_t x, std::size_t y) {
    return load(star_id, planet_order,
                Coordinates{static_cast<int>(x), static_cast<int>(y)});
  }

  void save(starnum_t star_id, planetnum_t planet_order, Coordinates coords,
            const sector_struct& sector);
  void save(starnum_t star_id, planetnum_t planet_order, std::size_t x,
            std::size_t y, const sector_struct& sector) {
    save(star_id, planet_order,
         Coordinates{static_cast<int>(x), static_cast<int>(y)}, sector);
  }

  // Legacy methods (for backward compatibility during migration)
  std::optional<Sector> find_sector(starnum_t star_id, planetnum_t planet_order,
                                    Coordinates coords);
  std::optional<Sector> find_sector(starnum_t star_id, planetnum_t planet_order,
                                    int x, int y) {
    return find_sector(star_id, planet_order, Coordinates{x, y});
  }

  bool save_sector(const Sector& sector, starnum_t star_id,
                   planetnum_t planet_order, Coordinates coords);
  bool save_sector(const Sector& sector, starnum_t star_id,
                   planetnum_t planet_order, int x, int y) {
    return save_sector(sector, star_id, planet_order, Coordinates{x, y});
  }

  // Bulk operations for sector maps
  SectorMap load_map(const Planet& planet);
  bool save_map(const SectorMap& map);

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Sector& sector) const override;
  [[nodiscard]] std::optional<Sector>
  deserialize(const std::string& json_str) const override;

private:
  static std::vector<std::pair<std::string, KeyValue>>
  sector_keys(starnum_t star_id, planetnum_t planet_order, Coordinates coords) {
    return {{"star_id", star_id},
            {"planet_order", planet_order},
            {"xpos", coords.x},
            {"ypos", coords.y}};
  }
};

// ============================================================================
// CommodRepository - Repository for commodity market data
// ============================================================================
export class CommodRepository : public Repository<Commod> {
public:
  explicit CommodRepository(JsonStore& store)
      : Repository<Commod>(store, "tbl_commod") {}

  // Domain-specific methods
  std::optional<Commod> find_by_id(int id) {
    return find(id);
  }
  bool save(const Commod& commod) {
    return Repository<Commod>::save(commod.id(), commod);
  }
  void delete_commod(int id) {
    store.remove(table_name, id);
  }

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Commod& commod) const override;
  [[nodiscard]] std::optional<Commod>
  deserialize(const std::string& json_str) const override;
};

// ============================================================================
// BlockRepository - Repository for alliance block data
// ============================================================================
export class BlockRepository : public Repository<Block> {
public:
  explicit BlockRepository(JsonStore& store)
      : Repository<Block>(store, "tbl_block") {}

  // Domain-specific methods
  std::optional<Block> find_by_id(player_t id) {
    return find(id);
  }
  bool save(const Block& b) {
    return Repository<Block>::save(b.Playernum(), b);
  }

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Block& b) const override;
  [[nodiscard]] std::optional<Block>
  deserialize(const std::string& json_str) const override;
};

// ============================================================================
// PowerRepository - Repository for player power statistics
// ============================================================================
export class PowerRepository : public Repository<Power> {
public:
  explicit PowerRepository(JsonStore& store)
      : Repository<Power>(store, "tbl_power") {}

  // Domain-specific methods
  std::optional<Power> find_by_id(player_t id) {
    return find(id);
  }
  bool save(const Power& p) {
    return Repository<Power>::save(p.id(), p);
  }

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Power& p) const override;
  [[nodiscard]] std::optional<Power>
  deserialize(const std::string& json_str) const override;
};

// ============================================================================
// UniverseRepository - Repository for global universe-wide statistics
// ============================================================================
export class UniverseRepository : public Repository<Universe> {
public:
  explicit UniverseRepository(JsonStore& store)
      : Repository<Universe>(store, "tbl_universe") {}

  // Domain-specific methods
  // Note: Universe is a singleton (id=1)
  std::optional<Universe> get_global_data() {
    return find(1);
  }
  bool save(const Universe& universe) {
    return Repository<Universe>::save(1, universe);
  }

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const Universe& universe) const override;
  [[nodiscard]] std::optional<Universe>
  deserialize(const std::string& json_str) const override;
};

// ============================================================================
// ServerStateRepository - Repository for server scheduling state
// ============================================================================
export class ServerStateRepository : public Repository<ServerState> {
public:
  explicit ServerStateRepository(JsonStore& store)
      : Repository<ServerState>(store, "tbl_server_state") {}

  // Domain-specific methods
  // Note: ServerState is a singleton (id=1)
  std::optional<ServerState> get_state() {
    return find(1);
  }
  bool save(const ServerState& state) {
    return Repository<ServerState>::save(1, state);
  }

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const ServerState& state) const override;
  [[nodiscard]] std::optional<ServerState>
  deserialize(const std::string& json_str) const override;
};

// ============================================================================
// ShipExamRepository - Repository for ship examination descriptions
// ============================================================================
export class ShipExamRepository : public Repository<ShipExam> {
public:
  explicit ShipExamRepository(JsonStore& store)
      : Repository<ShipExam>(store, "tbl_ship_exam") {}

  // Domain-specific methods
  std::optional<ShipExam> find_by_type(ShipType ship_type) {
    return find(std::to_underlying(ship_type));
  }
  bool save(const ShipExam& exam) {
    return Repository<ShipExam>::save(std::to_underlying(exam.ship_type), exam);
  }

  bool seed_from_file(const std::string& path);

protected:
  [[nodiscard]] std::optional<std::string>
  serialize(const ShipExam& exam) const override;
  [[nodiscard]] std::optional<ShipExam>
  deserialize(const std::string& json_str) const override;
};

// ============================================================================
// NewsRepository - Repository for news items
// ============================================================================
export class NewsRepository {
private:
  Database& db;

public:
  explicit NewsRepository(Database& database) : db(database) {}

  news_id_t add(NewsType type, std::string_view message) {
    const auto now = std::chrono::system_clock::now();
    const auto timestamp =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
            .count();

    return db.news_add(std::to_underlying(type), std::string(message),
                       timestamp);
  }

  std::vector<NewsItem> get_since(NewsType type,
                                  news_id_t since_id = news_id_t{0}) {
    return db.news_get_since(std::to_underlying(type), since_id);
  }

  bool purge_type(NewsType type) {
    return db.news_purge_type(std::to_underlying(type));
  }

  bool purge_all() {
    return db.news_purge_all();
  }
};

// ============================================================================
// TelegramRepository - Repository for telegram items
// ============================================================================
export class TelegramRepository {
private:
  Database& db;

public:
  explicit TelegramRepository(Database& database) : db(database) {}

  telegram_id_t add(player_t player, governor_t governor,
                    std::string_view message) {
    const auto now = std::chrono::system_clock::now();
    const auto timestamp =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
            .count();

    return db.telegram_add(player, governor, std::string(message), timestamp);
  }

  std::vector<TelegramItem> get(player_t player, governor_t governor) {
    return db.telegram_get(player, governor);
  }

  void delete_for_governor(player_t player, governor_t governor) {
    db.telegram_delete_for_governor(player, governor);
  }

  int count(player_t player, governor_t governor) {
    return db.telegram_count(player, governor);
  }

  bool purge_all() {
    return db.telegram_purge_all();
  }
};
