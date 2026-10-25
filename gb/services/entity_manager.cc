// SPDX-License-Identifier: Apache-2.0

/// \file entity_manager.cc
/// \brief Implementation of EntityManager caching, transactions, and DAL
/// persistence.

module;

import std;

module gb.services;

import dallib;
import gb.repositories;

namespace {
// Private template helpers for reducing code duplication
// These implement the common pattern: check cache, load if needed, manage
// refcounts

template <typename Entity, typename Key, typename FindFn>
Entity*
peek_entity_impl(Key key,
                 std::unordered_map<Key, std::unique_ptr<Entity>>& cache,
                 FindFn find_fn) {
  // Check if already cached
  auto it = cache.find(key);
  if (it != cache.end()) {
    return it->second.get();
  }

  // Load from repository if not cached
  auto entity_opt = find_fn(key);
  if (!entity_opt) {
    return nullptr;
  }

  // Cache the entity without incrementing refcount (read-only peek)
  auto [iter, inserted] =
      cache.emplace(key, std::make_unique<Entity>(std::move(*entity_opt)));
  return iter->second.get();
}

template <typename Entity, typename Key, typename FindFn, typename SaveFn,
          typename ReleaseFn>
EntityHandle<Entity>
get_entity_impl(EntityManager* manager, Key key,
                std::unordered_map<Key, std::unique_ptr<Entity>>& cache,
                std::unordered_map<Key, int>& refcount, FindFn find_fn,
                SaveFn save_fn, ReleaseFn release_fn) {
  Entity* entity = peek_entity_impl(key, cache, std::move(find_fn));
  if (!entity) {
    return {manager, nullptr, [](const Entity&) {}};
  }
  refcount[key]++;
  return {manager, entity,
          [manager, save_fn](const Entity& e) {
            if (!manager->is_deferred_write()) {
              save_fn(e);
            }
          },
          [release_fn, key]() { release_fn(key); }};
}

template <typename Entity, typename Key>
void release_entity_impl(
    Key key, std::unordered_map<Key, std::unique_ptr<Entity>>& /*cache*/,
    std::unordered_map<Key, int>& refcount) {
  auto it = refcount.find(key);
  if (it != refcount.end()) {
    it->second--;
    if (it->second <= 0) {
      refcount.erase(it);
    }
  }
}

template <typename Entity, typename Key>
void clear_cache_impl(std::unordered_map<Key, std::unique_ptr<Entity>>& cache,
                      std::unordered_map<Key, int>& refcount) {
  std::erase_if(cache, [&refcount](const auto& entry) {
    return !refcount.contains(entry.first);
  });
}

template <typename Entity, typename Key, typename SaveFn>
void flush_cache_impl(
    const std::unordered_map<Key, std::unique_ptr<Entity>>& cache,
    SaveFn save_fn) {
  for (const auto& [key, entity] : cache) {
    save_fn(*entity);
  }
}
}  // namespace

struct EntityManager::Storage {
  Database& db;
  JsonStore store;
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

  explicit Storage(Database& database)
      : db(database), store(database), races(store), ships(store),
        planets(store), stars(store), sectors(store), commods(store),
        blocks(store), powers(store), universe_repo(store),
        server_state_repo(store), ship_exams(store), news(store),
        telegrams(store) {}
};

EntityManager::EntityManager(Database& database)
    : storage_(std::make_unique<Storage>(database)) {}

EntityManager::~EntityManager() = default;

// Race entity methods
EntityHandle<Race> EntityManager::get_race(player_t player) {
  auto handle = get_entity_impl<Race>(
      this, player, race_cache, race_refcount,
      [this](player_t p) { return storage_->races.find_by_player(p); },
      [this](const Race& r) { storage_->races.save(r); },
      [this](player_t p) { release_race(p); });
  if (!handle.get()) {
    throw EntityNotFoundError(std::format("Race not found: player={}", player));
  }
  return handle;
}

const Race* EntityManager::peek_race(player_t player) {
  const auto* race =
      peek_entity_impl<Race>(player, race_cache, [this](player_t p) {
        return storage_->races.find_by_player(p);
      });
  if (!race) {
    throw EntityNotFoundError(std::format("Race not found: player={}", player));
  }
  return race;
}

void EntityManager::release_race(player_t player) {
  release_entity_impl<Race>(player, race_cache, race_refcount);
}

EntityHandle<Race> EntityManager::create_race(const Race& race_data) {
  player_t player = race_data.Playernum != 0
                        ? race_data.Playernum
                        : player_t{max_race_player().value + 1};
  Race new_race = race_data;
  new_race.Playernum = player;

  // Save through repository (DAL)
  storage_->races.save(new_race);

  // Auto-seed baseline block if not existing (or update name if uncustomized)
  if (auto existing_block = storage_->blocks.find_by_id(player);
      !existing_block) {
    storage_->blocks.save(Block{{.Playernum = player, .name = new_race.name}});
  } else if (existing_block->motto().empty()) {
    existing_block->name() = new_race.name;
    storage_->blocks.save(*existing_block);
  }

  // Auto-seed baseline power if not existing
  if (!storage_->powers.find_by_id(player)) {
    storage_->powers.save(Power{{.id = player}});
  }

  race_cache.insert_or_assign(player, std::make_unique<Race>(new_race));
  return get_race(player);
}

// Ship entity methods
EntityHandle<Ship> EntityManager::get_ship(shipnum_t num) {
  auto it = ship_cache.find(num);
  if (it == ship_cache.end()) {
    if (std::ranges::contains(pending_ship_deletions_, num)) {
      throw EntityNotFoundError(std::format("Ship not found: ship_id={}", num));
    }
    auto ship_ptr = storage_->ships.find_ship(num);
    if (!ship_ptr) {
      throw EntityNotFoundError(std::format("Ship not found: ship_id={}", num));
    }
    it = ship_cache.emplace(num, std::move(ship_ptr)).first;
  }
  ship_refcount[num]++;
  return {this, it->second.get(),
          [this](const Ship& s) {
            if (!is_deferred_write() &&
                !std::ranges::contains(pending_ship_deletions_, s.number())) {
              storage_->ships.save(s);
            }
          },
          [this, num]() { release_ship(num); }};
}

const Ship* EntityManager::peek_ship(shipnum_t num) {
  auto it = ship_cache.find(num);
  if (it != ship_cache.end()) {
    return it->second.get();
  }

  if (std::ranges::contains(pending_ship_deletions_, num)) {
    throw EntityNotFoundError(std::format("Ship not found: ship_id={}", num));
  }

  auto ship_ptr = storage_->ships.find_ship(num);
  if (!ship_ptr) {
    throw EntityNotFoundError(std::format("Ship not found: ship_id={}", num));
  }

  auto [iter, inserted] = ship_cache.emplace(num, std::move(ship_ptr));
  return iter->second.get();
}

void EntityManager::release_ship(shipnum_t num) {
  release_entity_impl<Ship>(num, ship_cache, ship_refcount);
  if (deletion_barrier_active_) return;
  if (ship_refcount.contains(num)) return;
  auto it = std::ranges::find(pending_ship_deletions_, num);
  if (it != pending_ship_deletions_.end()) {
    ship_cache.erase(num);
    if (!is_deferred_write()) {
      pending_ship_deletions_.erase(it);
      storage_->ships.delete_ship(num);
    }
  }
}

EntityHandle<Ship> EntityManager::create_ship(std::unique_ptr<Ship> ship) {
  if (!ship) {
    throw std::invalid_argument("create_ship: null ship provided");
  }

  // Get next available ship number if not explicitly specified
  shipnum_t num =
      ship->number() != 0 ? ship->number() : storage_->ships.next_ship_number();
  ship->number() = num;
  std::erase(pending_ship_deletions_, num);

  // Save immediately to database
  storage_->ships.save(*ship);

  ship_cache.insert_or_assign(num, std::move(ship));
  return get_ship(num);
}

EntityHandle<Ship> EntityManager::create_ship(ShipType type, player_t owner) {
  return create_ship(ShipFactory::create_from_template(type, owner));
}

void EntityManager::delete_ship(shipnum_t num) {
  if (!deletion_barrier_active_) {
    throw DeletionBarrierRequiredError(std::format(
        "delete_ship({}) called without an active DeletionBarrier", num));
  }
  pending_ship_deletions_.push_back(num);
}

void EntityManager::delete_commod(int id) {
  if (!deletion_barrier_active_) {
    throw DeletionBarrierRequiredError(std::format(
        "delete_commod({}) called without an active DeletionBarrier", id));
  }
  pending_commod_deletions_.push_back(id);
}

EntityHandle<Commod> EntityManager::create_commod(commod_struct init_data) {
  // Get next available commod ID
  int id = storage_->commods.next_available_id();
  std::erase(pending_commod_deletions_, id);

  // Assign ID and construct validated Commod
  init_data.id = id;
  Commod new_commod{init_data};

  // Save through repository (DAL)
  storage_->commods.save(new_commod);

  commod_cache.insert_or_assign(
      id, std::make_unique<Commod>(std::move(new_commod)));
  return get_commod(id);
}

// Planet entity methods
EntityHandle<Planet> EntityManager::get_planet(starnum_t star,
                                               planetnum_t pnum) {
  auto key = std::make_pair(star, pnum);
  auto handle = get_entity_impl<Planet>(
      this, key, planet_cache, planet_refcount,
      [this, star, pnum](auto) {
        return storage_->planets.find_by_location(star, pnum);
      },
      [this](const Planet& p) { storage_->planets.save(p); },
      [this, star, pnum](auto) { release_planet(star, pnum); });
  if (!handle.get()) {
    throw EntityNotFoundError(
        std::format("Planet not found: star_id={}, planet_id={}", star, pnum));
  }
  return handle;
}

const Planet* EntityManager::peek_planet(starnum_t star, planetnum_t pnum) {
  auto key = std::make_pair(star, pnum);
  const auto* planet =
      peek_entity_impl<Planet>(key, planet_cache, [this, star, pnum](auto) {
        return storage_->planets.find_by_location(star, pnum);
      });
  if (!planet) {
    throw EntityNotFoundError(
        std::format("Planet not found: star_id={}, planet_id={}", star, pnum));
  }
  return planet;
}

void EntityManager::release_planet(starnum_t star, planetnum_t pnum) {
  release_entity_impl<Planet>(std::make_pair(star, pnum), planet_cache,
                              planet_refcount);
}

// Star entity methods
EntityHandle<Star> EntityManager::get_star(starnum_t num) {
  auto handle = get_entity_impl<Star>(
      this, num, star_cache, star_refcount,
      [this](starnum_t n) { return storage_->stars.find_by_number(n); },
      [this](const Star& s) { storage_->stars.save(s); },
      [this](starnum_t n) { release_star(n); });
  if (!handle.get()) {
    throw EntityNotFoundError(std::format("Star not found: star_id={}", num));
  }
  return handle;
}

const Star* EntityManager::peek_star(starnum_t num) {
  const auto* star =
      peek_entity_impl<Star>(num, star_cache, [this](starnum_t n) {
        return storage_->stars.find_by_number(n);
      });
  if (!star) {
    throw EntityNotFoundError(std::format("Star not found: star_id={}", num));
  }
  return star;
}

void EntityManager::release_star(starnum_t num) {
  release_entity_impl<Star>(num, star_cache, star_refcount);
}

// Commod entity methods
EntityHandle<Commod> EntityManager::get_commod(int id) {
  auto handle = get_entity_impl<Commod>(
      this, id, commod_cache, commod_refcount,
      [this](int i) -> std::optional<Commod> {
        if (std::ranges::contains(pending_commod_deletions_, i)) {
          return std::nullopt;
        }
        return storage_->commods.find_by_id(i);
      },
      [this](const Commod& c) {
        if (!std::ranges::contains(pending_commod_deletions_, c.id())) {
          storage_->commods.save(c);
        }
      },
      [this](int i) { release_commod(i); });
  if (!handle.get()) {
    throw EntityNotFoundError(std::format("Commodity not found: id={}", id));
  }
  return handle;
}

const Commod* EntityManager::peek_commod(int id) {
  const auto* commod = peek_entity_impl<Commod>(
      id, commod_cache, [this](int i) -> std::optional<Commod> {
        if (std::ranges::contains(pending_commod_deletions_, i)) {
          return std::nullopt;
        }
        return storage_->commods.find_by_id(i);
      });
  if (!commod) {
    throw EntityNotFoundError(std::format("Commodity not found: id={}", id));
  }
  return commod;
}

void EntityManager::release_commod(int id) {
  release_entity_impl<Commod>(id, commod_cache, commod_refcount);
}

// Block entity methods
EntityHandle<Block> EntityManager::get_block(player_t id) {
  auto handle = get_entity_impl<Block>(
      this, id, block_cache, block_refcount,
      [this](player_t i) { return storage_->blocks.find_by_id(i); },
      [this](const Block& b) { storage_->blocks.save(b); },
      [this](player_t i) { release_block(i); });
  if (!handle.get()) {
    throw EntityNotFoundError(std::format("Block not found: id={}", id));
  }
  return handle;
}

const Block* EntityManager::peek_block(player_t id) {
  const auto* b = peek_entity_impl<Block>(id, block_cache, [this](player_t i) {
    return storage_->blocks.find_by_id(i);
  });
  if (!b) {
    throw EntityNotFoundError(std::format("Block not found: id={}", id));
  }
  return b;
}

void EntityManager::release_block(player_t id) {
  release_entity_impl<Block>(id, block_cache, block_refcount);
}

// Power entity methods
EntityHandle<Power> EntityManager::get_power(player_t id) {
  auto handle = get_entity_impl<Power>(
      this, id, power_cache, power_refcount,
      [this](player_t i) { return storage_->powers.find_by_id(i); },
      [this](const Power& p) { storage_->powers.save(p); },
      [this](player_t i) { release_power(i); });
  if (!handle.get()) {
    throw EntityNotFoundError(std::format("Power not found: id={}", id));
  }
  return handle;
}

const Power* EntityManager::peek_power(player_t id) {
  const auto* p = peek_entity_impl<Power>(id, power_cache, [this](player_t i) {
    return storage_->powers.find_by_id(i);
  });
  if (!p) {
    throw EntityNotFoundError(std::format("Power not found: id={}", id));
  }
  return p;
}

void EntityManager::release_power(player_t id) {
  release_entity_impl<Power>(id, power_cache, power_refcount);
}

// Universe entity methods (singleton)
EntityHandle<Universe> EntityManager::get_universe() {
  if (!global_universe_cache) {
    auto universe_opt = storage_->universe_repo.get_global_data();
    if (!universe_opt) {
      throw EntityNotFoundError("Universe not found");
    }
    global_universe_cache = std::make_unique<Universe>(*universe_opt);
  }
  global_universe_refcount++;
  return {this, global_universe_cache.get(),
          [this](const Universe& sd) {
            if (!is_deferred_write()) {
              storage_->universe_repo.save(sd);
            }
          },
          [this]() { release_universe(); }};
}

const Universe* EntityManager::peek_universe() {
  // Check if already cached
  if (global_universe_cache) {
    return global_universe_cache.get();
  }

  // Load from repository if not cached
  auto universe_opt = storage_->universe_repo.get_global_data();
  if (!universe_opt) {
    throw EntityNotFoundError("Universe not found");
  }

  // Cache the entity (but don't increment refcount - this is read-only)
  global_universe_cache = std::make_unique<Universe>(*universe_opt);
  return global_universe_cache.get();
}

int EntityManager::count_non_asteroid_planets() {
  return storage_->db.count_non_asteroid_planets();
}

void EntityManager::release_universe() {
  if (global_universe_refcount > 0) {
    global_universe_refcount--;
  }
}

// ServerState entity methods (singleton)
EntityHandle<ServerState> EntityManager::get_server_state() {
  bool created_default = false;
  if (!server_state_cache) {
    if (auto state_opt = storage_->server_state_repo.get_state()) {
      server_state_cache = std::make_unique<ServerState>(*state_opt);
    } else {
      server_state_cache = std::make_unique<ServerState>();
      created_default = true;
    }
  }
  server_state_refcount++;
  return {this, server_state_cache.get(),
          [this](const ServerState& state) {
            if (!is_deferred_write()) {
              storage_->server_state_repo.save(state);
            }
          },
          [this]() { release_server_state(); }, created_default};
}

const ServerState* EntityManager::peek_server_state() {
  // Check if already cached
  if (server_state_cache) {
    return server_state_cache.get();
  }

  // Load from repository if not cached
  auto state_opt = storage_->server_state_repo.get_state();
  if (!state_opt) {
    // If not in DB, create default state
    ServerState default_state{};
    server_state_cache = std::make_unique<ServerState>(default_state);
    return server_state_cache.get();
  }

  // Cache the entity (but don't increment refcount - this is read-only)
  server_state_cache = std::make_unique<ServerState>(*state_opt);
  return server_state_cache.get();
}

void EntityManager::release_server_state() {
  if (server_state_refcount > 0) {
    server_state_refcount--;
  }
}

namespace {
ShipExam load_or_create_ship_exam(ShipExamRepository& repo,
                                  ShipType ship_type) {
  if (auto existing = repo.find_by_type(ship_type)) {
    return std::move(*existing);
  }
  const auto& tmpl = ship_template(ship_type);
  ShipExam new_exam{.ship_type = ship_type,
                    .name = std::string(tmpl.name),
                    .description = std::format("{}\n", tmpl.name)};
  repo.save(new_exam);
  return new_exam;
}
}  // namespace

EntityHandle<ShipExam> EntityManager::get_ship_exam(ShipType ship_type) {
  return get_entity_impl<ShipExam>(
      this, ship_type, ship_exam_cache, ship_exam_refcount,
      [this](ShipType type) {
        return std::optional<ShipExam>(
            load_or_create_ship_exam(storage_->ship_exams, type));
      },
      [this](const ShipExam& exam) { storage_->ship_exams.save(exam); },
      [this](ShipType type) { release_ship_exam(type); });
}

const ShipExam* EntityManager::peek_ship_exam(ShipType ship_type) {
  return peek_entity_impl<ShipExam>(
      ship_type, ship_exam_cache, [this](ShipType type) {
        return std::optional<ShipExam>(
            load_or_create_ship_exam(storage_->ship_exams, type));
      });
}

void EntityManager::release_ship_exam(ShipType ship_type) {
  release_entity_impl<ShipExam>(ship_type, ship_exam_cache, ship_exam_refcount);
}

// Query methods
int EntityManager::num_commods() {
  // Count commods by listing all IDs in the database
  return storage_->store.list_ids("tbl_commod").size();
}

int EntityManager::max_commod_id() {
  auto ids = storage_->store.list_ids("tbl_commod");
  return ids.empty() ? 0 : ids.back();
}

int EntityManager::next_available_commod_id() {
  return storage_->commods.next_available_id();
}

player_t EntityManager::num_races() {
  // Count races by listing all IDs in the database
  return storage_->store.list_ids("tbl_race").size();
}

player_t EntityManager::max_race_player() {
  auto ids = storage_->store.list_ids("tbl_race");
  return ids.empty() ? player_t{0} : player_t{ids.back()};
}

starnum_t EntityManager::num_stars() {
  return starnum_t{static_cast<starnum_t::value_type>(
      storage_->store.list_ids("tbl_star").size())};
}

starnum_t EntityManager::max_star_id() {
  auto ids = storage_->store.list_ids("tbl_star");
  return ids.empty()
             ? starnum_t{0}
             : starnum_t{static_cast<starnum_t::value_type>(ids.back())};
}

shipnum_t EntityManager::num_ships() {
  // Count ships by listing all IDs in the database
  return storage_->store.list_ids("tbl_ship").size();
}

shipnum_t EntityManager::next_available_ship_number() {
  return storage_->ships.next_ship_number();
}

shipnum_t EntityManager::max_ship_number() {
  auto ids = storage_->store.list_ids("tbl_ship");
  return ids.empty()
             ? shipnum_t{0}
             : shipnum_t{static_cast<shipnum_t::value_type>(ids.back())};
}

player_t EntityManager::num_blocks() {
  return player_t{
      static_cast<int>(storage_->store.list_ids("tbl_block").size())};
}

player_t EntityManager::max_block_id() {
  auto ids = storage_->store.list_ids("tbl_block");
  return ids.empty() ? player_t{0} : player_t{ids.back()};
}

player_t EntityManager::num_powers() {
  return player_t{
      static_cast<int>(storage_->store.list_ids("tbl_power").size())};
}

player_t EntityManager::max_power_id() {
  auto ids = storage_->store.list_ids("tbl_power");
  return ids.empty() ? player_t{0} : player_t{ids.back()};
}

std::vector<shipnum_t>
EntityManager::filter_alive_ships(std::vector<shipnum_t> ids) const {
  std::erase_if(ids, [this](shipnum_t id) {
    if (std::ranges::contains(pending_ship_deletions_, id)) {
      return true;
    }
    if (auto it = ship_cache.find(id); it != ship_cache.end()) {
      return !it->second->alive();
    }
    return false;
  });
  return ids;
}

std::vector<shipnum_t> EntityManager::ships_in_star_system(starnum_t star_id) {
  return filter_alive_ships(storage_->ships.find_in_star_system(star_id));
}

std::vector<shipnum_t> EntityManager::ships_in_star(starnum_t star_id) {
  return filter_alive_ships(storage_->ships.find_in_star(star_id));
}

std::vector<shipnum_t> EntityManager::ships_on_planet(starnum_t star_id,
                                                      planetnum_t planet_id) {
  return filter_alive_ships(storage_->ships.find_on_planet(star_id, planet_id));
}

std::vector<shipnum_t> EntityManager::ships_in_hangar(shipnum_t carrier_id) {
  return filter_alive_ships(storage_->ships.find_in_hangar(carrier_id));
}

std::vector<shipnum_t> EntityManager::ships_by_owner(player_t owner_id) {
  return filter_alive_ships(storage_->ships.find_by_owner(owner_id));
}

std::vector<shipnum_t> EntityManager::ships_at_scope(ScopeLevel scope) {
  return filter_alive_ships(storage_->ships.find_at_scope(scope));
}

std::vector<shipnum_t> EntityManager::ships_alive() {
  return filter_alive_ships(storage_->ships.find_alive());
}

std::vector<shipnum_t> EntityManager::ships_all() {
  auto ids = storage_->store.list_ids("tbl_ship");
  std::vector<shipnum_t> result;
  result.reserve(ids.size());
  for (int id : ids) {
    shipnum_t snum = id;
    if (!std::ranges::contains(pending_ship_deletions_, snum)) {
      result.push_back(snum);
    }
  }
  return result;
}

// Utility methods
void EntityManager::flush_all() {
  // Save all cached entities - entities now contain their own IDs
  flush_cache_impl<Race>(race_cache,
                         [this](const Race& r) { storage_->races.save(r); });
  flush_cache_impl<Ship>(ship_cache, [this](const Ship& s) {
    if (!std::ranges::contains(pending_ship_deletions_, s.number())) {
      storage_->ships.save(s);
    }
  });
  flush_cache_impl<Planet>(
      planet_cache, [this](const Planet& p) { storage_->planets.save(p); });
  flush_cache_impl<Star>(star_cache,
                         [this](const Star& s) { storage_->stars.save(s); });
  flush_cache_impl<SectorMap>(sectormap_cache, [this](const SectorMap& sm) {
    storage_->sectors.save_map(sm);
  });
  flush_cache_impl<Commod>(commod_cache, [this](const Commod& c) {
    if (!std::ranges::contains(pending_commod_deletions_, c.id())) {
      storage_->commods.save(c);
    }
  });
  flush_cache_impl<Block>(block_cache,
                          [this](const Block& b) { storage_->blocks.save(b); });
  flush_cache_impl<Power>(power_cache,
                          [this](const Power& p) { storage_->powers.save(p); });
  flush_cache_impl<ShipExam>(ship_exam_cache, [this](const ShipExam& e) {
    storage_->ship_exams.save(e);
  });

  if (global_universe_cache) {
    storage_->universe_repo.save(*global_universe_cache);
  }

  if (server_state_cache) {
    storage_->server_state_repo.save(*server_state_cache);
  }

  for (shipnum_t num : pending_ship_deletions_) {
    ship_cache.erase(num);
    ship_refcount.erase(num);
    storage_->ships.delete_ship(num);
  }
  pending_ship_deletions_.clear();

  for (int id : pending_commod_deletions_) {
    commod_cache.erase(id);
    commod_refcount.erase(id);
    storage_->commods.delete_commod(id);
  }
  pending_commod_deletions_.clear();
}

void EntityManager::optimize() {
  storage_->db.optimize();
}

void EntityManager::clear_cache() {
  // Clear entities that have no active handles (refcount == 0)
  // Keep entities with active handles to avoid breaking those handles
  clear_cache_impl<Race>(race_cache, race_refcount);
  clear_cache_impl<Ship>(ship_cache, ship_refcount);
  clear_cache_impl<Planet>(planet_cache, planet_refcount);
  clear_cache_impl<Star>(star_cache, star_refcount);
  clear_cache_impl<SectorMap>(sectormap_cache, sectormap_refcount);
  clear_cache_impl<Commod>(commod_cache, commod_refcount);
  clear_cache_impl<Block>(block_cache, block_refcount);
  clear_cache_impl<Power>(power_cache, power_refcount);
  clear_cache_impl<ShipExam>(ship_exam_cache, ship_exam_refcount);

  // Clear global Universe if no active handles
  if (global_universe_refcount == 0) {
    global_universe_cache.reset();
  }

  // Clear server state if no active handles
  if (server_state_refcount == 0) {
    server_state_cache.reset();
  }
}

// Business logic operations
std::optional<player_t>
EntityManager::find_player_by_name(const std::string& name) {
  if (name.empty()) return std::nullopt;

  for (const Race& race : RaceList::readonly(*this)) {
    if (name == race.name || name == std::format("{}", race.Playernum)) {
      return race.Playernum;
    }
  }
  return std::nullopt;
}

void EntityManager::kill_ship(player_t Playernum, Ship& ship) {
  if (auto* auto_ship = ship.as<AutonomousShip>()) {
    auto_ship->set_who_killed(Playernum);
  }
  ship.alive() = 0;
  ship.notified() = 0; /* prepare the ship for recycling */
  if (ship.number() != 0) {
    std::erase(pending_ship_deletions_, ship.number());
    pending_ship_deletions_.push_back(ship.number());
  }

  if (ship.type() != ShipType::STYPE_POD &&
      ship.type() != ShipType::OTYPE_FACTORY) {
    /* pods don't do things to morale, ditto for factories */
    mutate_race(ship.owner(), [&](Race& victim) {
      if (victim.Gov_ship == ship.number()) victim.Gov_ship = std::nullopt;
      if (Playernum != ship.owner()) {
        if (!victim.God) {
          if (ship.type() != ShipType::OTYPE_VN) {
            mutate_race(Playernum, [&](Race& killer) {
              killer.adjust_morale(victim, static_cast<int>(ship.build_cost()));
            });
          }
        }
      } else if (!ship.docked() && ship.max_crew_capacity()) {
        victim.morale -= 2L * ship.build_cost(); /* scuttle/scrap */
      }
    });
  }

  if (const auto* auto_ship = ship.as<AutonomousShip>()) {
    mutate_universe([&](Universe& Sdata) {
      if (const auto killer = auto_ship->who_killed()) {
        Sdata.record_vn_kill(*killer, ship.orbited_star(),
                             uniform_rand(0, 1) == 0);
      }
    });
  }

  if (const auto* tox = ship.as<ToxicWasteShip>()) {
    if (ship.whatorbits() == ScopeLevel::LEVEL_PLAN) {
      mutate_planet(ship.storbits(), ship.pnumorbits(), [&](Planet& planet) {
        planet.toxic() += tox->toxic_level();
      });
    }
  }

  /* undock the stuff docked with it */
  if (ship.moored_ship_id()) {
    ship.undock_from_ship();
  }

  /* if this ship is inside a carrier, adjust the carrier's hangar and mass */
  if (ship.whatorbits() == ScopeLevel::LEVEL_SHIP) {
    const shipnum_t carrier_id = ship.destshipno().value();
    mutate_ship(carrier_id,
                [&](Ship& carrier) { carrier.unload_docked_craft(ship); });
    propagate_ancestor_mass_delta(carrier_id, -ship.mass());
  }

  /* landed ships are killed */
  for (auto ship_handle : ShipList::in_carrier(*this, ship.number())) {
    Ship& s = *ship_handle;   // Get mutable reference
    kill_ship(Playernum, s);  // Recursive call to member function
  }

  ship.clear_destination();

  /* clear any foreign key references to this ship on other alive ships */
  for (shipnum_t other_id : ships_alive()) {
    const auto* other = peek_ship(other_id);
    if (const auto* mirror = other->as<SpaceMirrorShip>()) {
      if (mirror->aimed_ship() == ship.number()) {
        mutate_as<SpaceMirrorShip>(other_id,
                                   [](SpaceMirrorShip& m) { m.clear_aim(); });
      }
    } else if (const auto* trans = other->as<TransporterShip>()) {
      if (trans->target_ship() == ship.number()) {
        mutate_as<TransporterShip>(other_id, [](TransporterShip& t) {
          t.set_target_ship(std::nullopt);
        });
      }
    }
    if (other->protect().ship == ship.number()) {
      mutate_ship(other_id, [](Ship& s) {
        s.protect().ship = std::nullopt;
        s.protect().on = false;
      });
    }
    if (other->destshipno() == ship.number()) {
      mutate_ship(other_id, [](Ship& s) {
        if (s.is_docked()) {
          s.undock_from_ship();
        } else {
          s.clear_destination();
        }
      });
    }
  }

  if (ship.number() != 0) {
    std::erase(pending_ship_deletions_, ship.number());
    pending_ship_deletions_.push_back(ship.number());
  }
  if (!ship_refcount.contains(ship.number())) {
    release_ship(ship.number());
  }
}

void EntityManager::propagate_ancestor_mass_delta(shipnum_t direct_carrier_id,
                                                  double delta_mass) {
  if (std::abs(delta_mass) < 1e-9) return;
  const auto* current = peek_ship(direct_carrier_id);
  while (current->whatorbits() == ScopeLevel::LEVEL_SHIP) {
    const shipnum_t ancestor_id = current->destshipno().value();
    mutate_ship(ancestor_id, [&](Ship& anc) {
      anc.set_mass(std::max(anc.base_mass(), anc.mass() + delta_mass));
    });
    current = peek_ship(ancestor_id);
  }
}

std::expected<void, DockError>
EntityManager::dock_carrier(shipnum_t child_id, shipnum_t carrier_id) {
  if (child_id == carrier_id) {
    return std::unexpected(DockError::SelfDocking);
  }

  const auto* child_peek = peek_ship(child_id);
  const auto* carrier_peek = peek_ship(carrier_id);

  // Cycle detection: ensure child_id is not an ancestor of carrier_id
  for (const auto* anc = carrier_peek;
       anc->whatorbits() == ScopeLevel::LEVEL_SHIP;
       anc = peek_ship(anc->destshipno().value())) {
    if (anc->destshipno() == child_id) {
      return std::unexpected(DockError::CycleDetected);
    }
  }

  if (child_peek->size() > carrier_peek->hanger_space()) {
    return std::unexpected(DockError::CarrierFull);
  }

  const double child_mass = child_peek->mass();

  // Atomically mutate carrier and child
  mutate_ship(carrier_id, [&](Ship& carrier) {
    mutate_ship(child_id, [&](Ship& child) {
      child.dock_into_carrier(carrier_id);
      carrier.load_docked_craft(child);
    });
  });

  // Propagate mass delta up ancestor carriers
  propagate_ancestor_mass_delta(carrier_id, child_mass);

  return {};
}

std::expected<void, UndockError>
EntityManager::undock_carrier(shipnum_t child_id, ScopeLevel orbit_level) {
  const auto* child_peek = peek_ship(child_id);
  const auto carrier_id_opt = child_peek->carrier_id();
  if (!carrier_id_opt) {
    return std::unexpected(UndockError::NotDocked);
  }

  const shipnum_t carrier_id = *carrier_id_opt;
  const double child_mass = child_peek->mass();

  mutate_ship(carrier_id, [&](Ship& carrier) {
    mutate_ship(child_id, [&](Ship& child) {
      carrier.unload_docked_craft(child);
      child.launch_to_orbit(orbit_level);
    });
  });

  // Propagate mass reduction up ancestor carriers
  propagate_ancestor_mass_delta(carrier_id, -child_mass);

  return {};
}

std::expected<void, DockError> EntityManager::moor_ships(shipnum_t ship1_id,
                                                         shipnum_t ship2_id) {
  if (ship1_id == ship2_id) {
    return std::unexpected(DockError::SelfDocking);
  }

  const auto* s1_peek = peek_ship(ship1_id);
  const auto* s2_peek = peek_ship(ship2_id);

  if (!s1_peek->is_spaceborne()) {
    return std::unexpected(DockError::NotSpaceborne);
  }
  if (!s2_peek->is_spaceborne()) {
    return std::unexpected(DockError::NotSpaceborne);
  }

  if (s1_peek->whatorbits() != s2_peek->whatorbits()) {
    return std::unexpected(DockError::ScopeMismatch);
  }

  mutate_ship(ship1_id, [&](Ship& s1) {
    mutate_ship(ship2_id, [&](Ship& s2) { s1.moor_together(s2); });
  });

  return {};
}

std::expected<void, UndockError>
EntityManager::unmoor_ships(shipnum_t ship_id) {
  const auto* s_peek = peek_ship(ship_id);
  const auto other_id = s_peek->moored_ship_id();
  if (!other_id) {
    return std::unexpected(UndockError::NotDocked);
  }

  mutate_ship(ship_id, [&](Ship& s1) { s1.undock_from_ship(); });
  mutate_ship(*other_id, [&](Ship& s2) {
    if (s2.destshipno() == ship_id) {
      s2.undock_from_ship();
    }
  });

  return {};
}

// SectorMap operations (cached with RAII like other entities)
EntityHandle<SectorMap> EntityManager::get_sectormap(starnum_t star,
                                                     planetnum_t pnum) {
  auto key = std::make_pair(star, pnum);
  return get_entity_impl<SectorMap>(
      this, key, sectormap_cache, sectormap_refcount,
      [this, star, pnum](auto) {
        const auto& planet = *peek_planet(star, pnum);
        return std::optional<SectorMap>(storage_->sectors.load_map(planet));
      },
      [this](const SectorMap& sm) { storage_->sectors.save_map(sm); },
      [this, star, pnum](auto) { release_sectormap(star, pnum); });
}

const SectorMap* EntityManager::peek_sectormap(starnum_t star,
                                               planetnum_t pnum) {
  auto key = std::make_pair(star, pnum);
  return peek_entity_impl<SectorMap>(
      key, sectormap_cache, [this, star, pnum](auto) {
        const auto& planet = *peek_planet(star, pnum);
        return std::optional<SectorMap>(storage_->sectors.load_map(planet));
      });
}

void EntityManager::release_sectormap(starnum_t star, planetnum_t pnum) {
  release_entity_impl<SectorMap>(std::make_pair(star, pnum), sectormap_cache,
                                 sectormap_refcount);
}

// News operations (Service Layer)
news_id_t EntityManager::post_news(NewsType type, std::string_view message) {
  return storage_->news.add(type, message);
}

std::vector<NewsItem> EntityManager::get_news_since(NewsType type,
                                                    news_id_t since_id) {
  return storage_->news.get_since(type, since_id);
}

void EntityManager::purge_news_type(NewsType type) {
  storage_->news.purge_type(type);
}

void EntityManager::purge_all_news() {
  storage_->news.purge_all();
}

// Telegram operations (Service Layer)
telegram_id_t EntityManager::post_telegram(player_t player, governor_t governor,
                                           std::string_view message) {
  return storage_->telegrams.add(player, governor, message);
}

std::vector<TelegramItem> EntityManager::get_telegrams(player_t player,
                                                       governor_t governor) {
  return storage_->telegrams.get(player, governor);
}

void EntityManager::delete_telegrams(player_t player, governor_t governor) {
  storage_->telegrams.delete_for_governor(player, governor);
}

bool EntityManager::has_telegrams(player_t player, governor_t governor) {
  return storage_->telegrams.count(player, governor) > 0;
}

void EntityManager::purge_all_telegrams() {
  storage_->telegrams.purge_all();
}

// Transaction implementation
EntityManager::Transaction::Transaction(EntityManager& em)
    : em_(&em), active_(true) {
  em_->storage_->db.begin_transaction();
}

EntityManager::Transaction::~Transaction() {
  if (active_) {
    try {
      rollback();
    } catch (...) {
      // Destructors must not throw exceptions
    }
  }
}

EntityManager::Transaction::Transaction(Transaction&& other) noexcept
    : em_(other.em_), active_(std::exchange(other.active_, false)) {}

EntityManager::Transaction&
EntityManager::Transaction::operator=(Transaction&& other) noexcept {
  if (this != &other) {
    if (active_) {
      try {
        rollback();
      } catch (...) {
      }
    }
    em_ = other.em_;
    active_ = std::exchange(other.active_, false);
  }
  return *this;
}

void EntityManager::Transaction::commit() {
  if (active_) {
    em_->storage_->db.commit();
    active_ = false;
  }
}

void EntityManager::Transaction::rollback() {
  if (active_) {
    em_->storage_->db.rollback();
    em_->clear_cache();
    active_ = false;
  }
}

EntityManager::Transaction EntityManager::begin_transaction() {
  return Transaction(*this);
}

// DeferredWriteScope implementation
EntityManager::DeferredWriteScope::DeferredWriteScope(EntityManager& em)
    : em_(&em), committed_(false) {
  em_->deferred_write_depth_++;
}

EntityManager::DeferredWriteScope::~DeferredWriteScope() noexcept(false) {
  if (!committed_) {
    if (std::uncaught_exceptions() > 0) {
      try {
        rollback();
      } catch (...) {
      }
    } else {
      commit();
    }
  }
}

EntityManager::DeferredWriteScope::DeferredWriteScope(
    DeferredWriteScope&& other) noexcept
    : em_(other.em_), committed_(std::exchange(other.committed_, true)) {}

EntityManager::DeferredWriteScope& EntityManager::DeferredWriteScope::operator=(
    DeferredWriteScope&& other) noexcept {
  if (this != &other) {
    if (!committed_) {
      try {
        if (std::uncaught_exceptions() > 0) {
          rollback();
        } else {
          commit();
        }
      } catch (...) {
      }
    }
    em_ = other.em_;
    committed_ = std::exchange(other.committed_, true);
  }
  return *this;
}

void EntityManager::DeferredWriteScope::commit() {
  if (!committed_) {
    committed_ = true;
    em_->deferred_write_depth_--;
    em_->storage_->db.begin_transaction();
    try {
      em_->flush_all();
    } catch (...) {
      try {
        em_->storage_->db.rollback();
      } catch (...) {
      }
      em_->pending_ship_deletions_.clear();
      em_->pending_commod_deletions_.clear();
      em_->clear_cache();
      throw;
    }
    em_->storage_->db.commit();
  }
}

void EntityManager::DeferredWriteScope::rollback() {
  if (!committed_) {
    em_->deferred_write_depth_--;
    em_->pending_ship_deletions_.clear();
    em_->pending_commod_deletions_.clear();
    em_->clear_cache();
    committed_ = true;
  }
}

EntityManager::DeferredWriteScope EntityManager::create_deferred_write_scope() {
  return DeferredWriteScope(*this);
}

void EntityManager::drain_pending_deletions() {
  for (shipnum_t num : pending_ship_deletions_) {
    if (auto ref_it = ship_refcount.find(num); ref_it != ship_refcount.end()) {
      throw EntityInUseError(std::format(
          "Cannot delete ship {}: still referenced by {} active handle(s)", num,
          ref_it->second));
    }
    ship_cache.erase(num);
    ship_refcount.erase(num);
    if (!is_deferred_write()) {
      storage_->ships.delete_ship(num);
    }
  }
  if (!is_deferred_write()) {
    pending_ship_deletions_.clear();
  }

  for (int id : pending_commod_deletions_) {
    if (auto ref_it = commod_refcount.find(id);
        ref_it != commod_refcount.end()) {
      throw EntityInUseError(
          std::format("Cannot delete commodity {}: still referenced by {} "
                      "active handle(s)",
                      id, ref_it->second));
    }
    commod_cache.erase(id);
    commod_refcount.erase(id);
    if (!is_deferred_write()) {
      storage_->commods.delete_commod(id);
    }
  }
  if (!is_deferred_write()) {
    pending_commod_deletions_.clear();
  }
}

// DeletionBarrier implementation
EntityManager::DeletionBarrier::DeletionBarrier(EntityManager& em) : em_(&em) {
  if (em_->deletion_barrier_active_) {
    throw std::logic_error(
        "A DeletionBarrier is already active on this EntityManager; nested "
        "barriers are not permitted");
  }
  em_->deletion_barrier_active_ = true;
}

EntityManager::DeletionBarrier::~DeletionBarrier() noexcept(false) {
  if (em_) {
    em_->deletion_barrier_active_ = false;
    if (std::uncaught_exceptions() > 0) {
      em_->pending_ship_deletions_.clear();
      em_->pending_commod_deletions_.clear();
    } else {
      em_->drain_pending_deletions();
    }
  }
}

EntityManager::DeletionBarrier::DeletionBarrier(
    DeletionBarrier&& other) noexcept
    : em_(std::exchange(other.em_, nullptr)) {}

EntityManager::DeletionBarrier&
EntityManager::DeletionBarrier::operator=(DeletionBarrier&& other) noexcept {
  if (this != &other) {
    if (em_) {
      em_->deletion_barrier_active_ = false;
      try {
        if (std::uncaught_exceptions() == 0) {
          em_->drain_pending_deletions();
        } else {
          em_->pending_ship_deletions_.clear();
          em_->pending_commod_deletions_.clear();
        }
      } catch (...) {
      }
    }
    em_ = std::exchange(other.em_, nullptr);
  }
  return *this;
}

EntityManager::DeletionBarrier EntityManager::create_deletion_barrier() {
  return DeletionBarrier(*this);
}

std::optional<UniverseCoordinates>
EntityManager::resolve_mirror_target_coordinates(
    const SpaceMirrorShip& mirror) {
  switch (mirror.aimed_level()) {
    case ScopeLevel::LEVEL_STAR:
      if (!mirror.aimed_star()) return std::nullopt;
      return peek_star(*mirror.aimed_star())->coordinates();
    case ScopeLevel::LEVEL_PLAN: {
      if (!mirror.aimed_planet()) return std::nullopt;
      const auto& star = *peek_star(*mirror.aimed_star());
      const auto& planet =
          *peek_planet(*mirror.aimed_star(), *mirror.aimed_planet());
      return planet.absolute_coordinates(star);
    }
    case ScopeLevel::LEVEL_SHIP: {
      if (!mirror.aimed_ship()) return std::nullopt;
      const auto& target_ship = *peek_ship(*mirror.aimed_ship());
      if (!target_ship.alive()) return std::nullopt;
      return target_ship.coordinates();
    }
    default:
      return std::nullopt;
  }
}

bearing_t
EntityManager::resolve_mirror_aim_direction(const SpaceMirrorShip& mirror) {
  const auto target_coords = resolve_mirror_target_coordinates(mirror);
  if (!target_coords) {
    return 0;
  }
  return mirror.aim_direction(*target_coords);
}
