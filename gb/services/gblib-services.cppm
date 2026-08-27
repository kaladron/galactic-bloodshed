// SPDX-License-Identifier: Apache-2.0

export module gblib:services;

import dallib;
import :repositories;
import :types;
import std;

// Exception thrown when an entity is not found in the database
// This represents a programming error or data corruption, not an expected
// condition
export class EntityNotFoundError : public std::runtime_error {
public:
  explicit EntityNotFoundError(const std::string& msg)
      : std::runtime_error(msg) {}
};

// Hash function for composite keys
namespace std {
template <>
struct hash<std::pair<starnum_t, planetnum_t>> {
  std::size_t operator()(const std::pair<starnum_t, planetnum_t>& p) const {
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
  EntityHandle(EntityManager* mgr, T* ent, std::function<void(const T&)> save,
               bool initial_dirty = false)
      : manager(mgr), entity(ent), save_fn(std::move(save)),
        dirty(initial_dirty) {}

  ~EntityHandle() {
    try {
      if (dirty && entity && save_fn) {
        save_fn(*entity);
      }
    } catch (...) {
      // Destructors must not throw exceptions
    }
    // Note: EntityManager will be notified via release mechanism
  }

  // Delete copy, allow move with proper nulling of moved-from source
  EntityHandle(const EntityHandle&) = delete;
  EntityHandle& operator=(const EntityHandle&) = delete;
  EntityHandle(EntityHandle&& other) noexcept
      : manager(other.manager), entity(std::exchange(other.entity, nullptr)),
        save_fn(std::move(other.save_fn)),
        dirty(std::exchange(other.dirty, false)) {}
  EntityHandle& operator=(EntityHandle&& other) noexcept {
    if (this != &other) {
      if (dirty && entity) {
        save_fn(*entity);
      }
      manager = other.manager;
      entity = std::exchange(other.entity, nullptr);
      save_fn = std::move(other.save_fn);
      dirty = std::exchange(other.dirty, false);
    }
    return *this;
  }

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
  [[nodiscard]] const T* get() const {
    return entity;
  }

  // Explicit read-only access (doesn't mark dirty)
  [[nodiscard]] const T& read() const {
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
  ServerStateRepository server_state_repo;
  ShipExamRepository ship_exams;
  NewsRepository news;
  TelegramRepository telegrams;

  // In-memory cache (only one copy of each entity)
  std::unordered_map<player_t, std::unique_ptr<Race>> race_cache;
  std::unordered_map<shipnum_t, std::unique_ptr<Ship>> ship_cache;
  std::unordered_map<std::pair<starnum_t, planetnum_t>, std::unique_ptr<Planet>>
      planet_cache;
  std::unordered_map<starnum_t, std::unique_ptr<Star>> star_cache;
  std::unordered_map<std::pair<starnum_t, planetnum_t>,
                     std::unique_ptr<SectorMap>>
      sectormap_cache;
  std::unordered_map<int, std::unique_ptr<Commod>> commod_cache;
  std::unordered_map<blocknum_t, std::unique_ptr<block>> block_cache;
  std::unordered_map<powernum_t, std::unique_ptr<power>> power_cache;
  std::unordered_map<ShipType, std::unique_ptr<ShipExam>> ship_exam_cache;
  std::unique_ptr<universe_struct> global_universe_cache;  // Singleton
  std::unique_ptr<ServerState> server_state_cache;         // Singleton

  // Reference counting for concurrent access
  std::unordered_map<player_t, int> race_refcount;
  std::unordered_map<shipnum_t, int> ship_refcount;
  std::unordered_map<std::pair<starnum_t, planetnum_t>, int> planet_refcount;
  std::unordered_map<starnum_t, int> star_refcount;
  std::unordered_map<std::pair<starnum_t, planetnum_t>, int> sectormap_refcount;
  std::unordered_map<int, int> commod_refcount;
  std::unordered_map<blocknum_t, int> block_refcount;
  std::unordered_map<powernum_t, int> power_refcount;
  std::unordered_map<ShipType, int> ship_exam_refcount;
  int global_universe_refcount = 0;
  int server_state_refcount = 0;

  // Mutex for thread-safety (future-proofing)
  std::mutex cache_mutex;

public:
  explicit EntityManager(Database& database);

  // Get entity handles (load from DB if not cached)
  // Throws EntityNotFoundError if entity not found
  EntityHandle<Race> get_race(player_t player);
  EntityHandle<Ship> get_ship(shipnum_t num);
  EntityHandle<Planet> get_planet(starnum_t star, planetnum_t pnum);
  EntityHandle<Star> get_star(starnum_t num);
  EntityHandle<Commod> get_commod(int id);
  EntityHandle<block> get_block(blocknum_t id);
  EntityHandle<power> get_power(powernum_t id);
  EntityHandle<universe_struct> get_universe();
  EntityHandle<ServerState> get_server_state();
  EntityHandle<ShipExam> get_ship_exam(ShipType ship_type);

  // Direct access for read-only operations (no RAII overhead)
  // Throws EntityNotFoundError if entity not found
  const Race* peek_race(player_t player);
  const Ship* peek_ship(shipnum_t num);
  const Planet* peek_planet(starnum_t star, planetnum_t pnum);
  const Star* peek_star(starnum_t num);
  const Commod* peek_commod(int id);
  const block* peek_block(blocknum_t id);
  const power* peek_power(powernum_t id);
  const universe_struct* peek_universe();
  const ServerState* peek_server_state();
  const ShipExam* peek_ship_exam(ShipType ship_type);

  // Sector map operations (cached with RAII like other entities)
  EntityHandle<SectorMap> get_sectormap(starnum_t star, planetnum_t pnum);
  const SectorMap* peek_sectormap(starnum_t star, planetnum_t pnum);

  // Scoped read-only access methods (with_* monadic helpers)
  template <typename Fn>
  decltype(auto) with_race(player_t player, Fn&& fn) {
    const auto* race = peek_race(player);
    return std::forward<Fn>(fn)(*race);
  }

  template <typename Fn>
  decltype(auto) with_ship(shipnum_t num, Fn&& fn) {
    const auto* ship = peek_ship(num);
    return std::forward<Fn>(fn)(*ship);
  }

  template <typename Fn>
  decltype(auto) with_planet(starnum_t star, planetnum_t pnum, Fn&& fn) {
    const auto* planet = peek_planet(star, pnum);
    return std::forward<Fn>(fn)(*planet);
  }

  template <typename Fn>
  decltype(auto) with_star(starnum_t num, Fn&& fn) {
    const auto* star = peek_star(num);
    return std::forward<Fn>(fn)(*star);
  }

  template <typename Fn>
  decltype(auto) with_sectormap(starnum_t star, planetnum_t pnum, Fn&& fn) {
    const auto* smap = peek_sectormap(star, pnum);
    return std::forward<Fn>(fn)(*smap);
  }

  template <typename Fn>
  decltype(auto) with_universe(Fn&& fn) {
    const auto* u = peek_universe();
    return std::forward<Fn>(fn)(*u);
  }

  template <typename Fn>
  decltype(auto) with_server_state(Fn&& fn) {
    const auto* ss = peek_server_state();
    return std::forward<Fn>(fn)(*ss);
  }

  template <typename Fn>
  decltype(auto) with_ship_exam(ShipType ship_type, Fn&& fn) {
    const auto* exam = peek_ship_exam(ship_type);
    return std::forward<Fn>(fn)(*exam);
  }

  template <typename Fn>
  decltype(auto) with_commod(int id, Fn&& fn) {
    const auto* commod = peek_commod(id);
    return std::forward<Fn>(fn)(*commod);
  }

  template <typename Fn>
  decltype(auto) with_block(blocknum_t id, Fn&& fn) {
    const auto* b = peek_block(id);
    return std::forward<Fn>(fn)(*b);
  }

  template <typename Fn>
  decltype(auto) with_power(powernum_t id, Fn&& fn) {
    const auto* p = peek_power(id);
    return std::forward<Fn>(fn)(*p);
  }

  // Create new entities
  EntityHandle<Ship> create_ship(const ship_struct& data = {});
  void delete_ship(shipnum_t num);
  EntityHandle<Commod> create_commod(const Commod& data = {});
  void delete_commod(int id);
  int next_available_commod_id();

  // Count and ID boundary methods (for queries and iteration)
  int num_commods();
  int max_commod_id();
  player_t num_races();
  player_t max_race_player();
  shipnum_t num_ships();
  shipnum_t max_ship_number();

  // Business logic operations (service layer)
  std::optional<player_t> find_player_by_name(const std::string& name);
  void kill_ship(player_t destroyer, Ship& ship);

  // News operations (service layer)
  void post_news(NewsType type, std::string_view message);
  std::vector<NewsItem> get_news_since(NewsType type, int since_id);
  int get_latest_news_id(NewsType type);
  void purge_news_type(NewsType type);
  void purge_all_news();

  // Telegram operations (service layer)
  void post_telegram(player_t player, governor_t governor,
                     std::string_view message);
  std::vector<TelegramItem> get_telegrams(player_t player, governor_t governor);
  void delete_telegrams(player_t player, governor_t governor);
  bool has_telegrams(player_t player, governor_t governor);
  void purge_all_telegrams();

  // Flush all dirty entities to database
  void flush_all();

  // Run database maintenance after turn processing
  void optimize();

  // Clear cache (for testing or after turn processing)
  void clear_cache();

private:
  // Release methods called by EntityHandle destructor
  void release_race(player_t player);
  void release_ship(shipnum_t num);
  void release_planet(starnum_t star, planetnum_t pnum);
  void release_star(starnum_t num);
  void release_commod(int id);
  void release_block(blocknum_t id);
  void release_power(powernum_t id);
  void release_universe();
  void release_server_state();
  void release_sectormap(starnum_t star, planetnum_t pnum);
  void release_ship_exam(ShipType ship_type);
};

export inline void record_vn_destruction_site(int& index1, int& index2,
                                              int star_id,
                                              bool supplant_first) {
  if (index1 != -1 && (index2 == -1 || !supplant_first)) {
    index2 = star_id;
  } else {
    index1 = star_id;
  }
}
