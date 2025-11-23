// SPDX-License-Identifier: Apache-2.0

export module gblib:services;

import dallib;
import :repositories;
import :types;
import std.compat;

// Hash function for composite keys
namespace std {
template <>
struct hash<std::pair<starnum_t, planetnum_t>> {
  size_t operator()(const std::pair<starnum_t, planetnum_t>& p) const {
    return std::hash<starnum_t>{}(p.first) ^
           (std::hash<planetnum_t>{}(p.second) << 1);
  }
};
}  // namespace std

// Forward declaration
export class EntityManager;

// RAII wrapper for entities - auto-saves on destruction if modified
export template <typename T>
class EntityHandle {
  EntityManager* manager;
  T* entity;
  std::function<void(const T&)> save_fn;
  bool dirty = false;

public:
  EntityHandle(EntityManager* mgr, T* ent, std::function<void(const T&)> save)
      : manager(mgr), entity(ent), save_fn(std::move(save)) {}

  ~EntityHandle() {
    if (dirty && entity) {
      save_fn(*entity);
    }
    // Note: EntityManager will be notified via release mechanism
  }

  // Delete copy, allow move
  EntityHandle(const EntityHandle&) = delete;
  EntityHandle& operator=(const EntityHandle&) = delete;
  EntityHandle(EntityHandle&&) noexcept = default;
  EntityHandle& operator=(EntityHandle&&) noexcept = default;

  // Non-const access marks entity as dirty
  T& operator*() {
    dirty = true;
    return *entity;
  }
  const T& operator*() const {
    return *entity;
  }

  T* operator->() {
    dirty = true;
    return entity;
  }
  const T* operator->() const {
    return entity;
  }

  T* get() {
    dirty = true;
    return entity;
  }
  const T* get() const {
    return entity;
  }

  // Explicit read-only access (doesn't mark dirty)
  const T& read() const {
    return *entity;
  }

  // Force save without waiting for destructor
  void save() {
    if (entity && dirty) {
      save_fn(*entity);
      dirty = false;
    }
  }
};

// Entity manager with caching and lifecycle management
export class EntityManager {
  Database& db;
  JsonStore store;

  // Repositories
  RaceRepository races;
  ShipRepository ships;
  PlanetRepository planets;
  StarRepository stars;
  SectorRepository sectors;
  CommodRepository commods;
  BlockRepository blocks;
  PowerRepository powers;
  UniverseRepository universe_repo;

  // In-memory cache (only one copy of each entity)
  std::unordered_map<player_t, std::unique_ptr<Race>> race_cache;
  std::unordered_map<shipnum_t, std::unique_ptr<Ship>> ship_cache;
  std::unordered_map<std::pair<starnum_t, planetnum_t>, std::unique_ptr<Planet>>
      planet_cache;
  std::unordered_map<starnum_t, std::unique_ptr<Star>> star_cache;
  // Note: Sectors are typically accessed in bulk via SectorMap, not cached
  // individually
  std::unordered_map<int, std::unique_ptr<Commod>> commod_cache;
  std::unordered_map<int, std::unique_ptr<block>> block_cache;
  std::unordered_map<int, std::unique_ptr<power>> power_cache;
  std::unique_ptr<universe_struct> global_universe_cache;  // Singleton

  // Reference counting for concurrent access
  std::unordered_map<player_t, int> race_refcount;
  std::unordered_map<shipnum_t, int> ship_refcount;
  std::unordered_map<std::pair<starnum_t, planetnum_t>, int> planet_refcount;
  std::unordered_map<starnum_t, int> star_refcount;
  std::unordered_map<int, int> commod_refcount;
  std::unordered_map<int, int> block_refcount;
  std::unordered_map<int, int> power_refcount;
  int global_universe_refcount = 0;

  // Mutex for thread-safety (future-proofing)
  std::mutex cache_mutex;

public:
  explicit EntityManager(Database& database);

  // Get entity handles (load from DB if not cached)
  EntityHandle<Race> get_race(player_t player);
  EntityHandle<Ship> get_ship(shipnum_t num);
  EntityHandle<Planet> get_planet(starnum_t star, planetnum_t pnum);
  EntityHandle<Star> get_star(starnum_t num);
  EntityHandle<Commod> get_commod(int id);
  EntityHandle<block> get_block(int id);
  EntityHandle<power> get_power(int id);
  EntityHandle<universe_struct> get_universe();

  // Direct access for read-only operations (no RAII overhead)
  const Race* peek_race(player_t player);
  const Ship* peek_ship(shipnum_t num);
  const Planet* peek_planet(starnum_t star, planetnum_t pnum);
  const Star* peek_star(starnum_t num);
  const universe_struct* peek_universe();

  // Create new entities
  EntityHandle<Ship> create_ship();
  void delete_ship(shipnum_t num);

  // Count methods (for queries)
  int num_commods();
  player_t num_races();
  shipnum_t num_ships();

  // Business logic operations (service layer)
  std::optional<player_t> find_player_by_name(const std::string& name);
  void kill_ship(player_t destroyer, Ship& ship);

  // Flush all dirty entities to database
  void flush_all();

  // Clear cache (for testing or after turn processing)
  void clear_cache();

private:
  // Release methods called by EntityHandle destructor
  void release_race(player_t player);
  void release_ship(shipnum_t num);
  void release_planet(starnum_t star, planetnum_t pnum);
  void release_star(starnum_t num);
  void release_commod(int id);
  void release_block(int id);
  void release_power(int id);
  void release_universe();
};
