// SPDX-License-Identifier: Apache-2.0

module;

import std.compat;

module gblib;

// Implementation of EntityManager class declared in gblib:services partition

namespace {
// Private template helpers for reducing code duplication
// These implement the common pattern: check cache, load if needed, manage
// refcounts

template <typename Entity, typename Key, typename FindFn, typename SaveFn,
          typename ReleaseFn>
EntityHandle<Entity>
get_entity_impl(EntityManager* manager, Key key,
                std::unordered_map<Key, std::unique_ptr<Entity>>& cache,
                std::unordered_map<Key, int>& refcount, FindFn find_fn,
                SaveFn save_fn, ReleaseFn release_fn) {
  // Check if already cached
  auto it = cache.find(key);
  if (it != cache.end()) {
    refcount[key]++;
    return {manager, it->second.get(),
            [save_fn, release_fn, key](const Entity& e) {
              save_fn(e);
              release_fn(key);
            }};
  }

  // Load from repository
  auto entity_opt = find_fn(key);
  if (!entity_opt) {
    return {manager, nullptr, [](const Entity&) {}};
  }

  // Cache the entity
  auto [iter, inserted] =
      cache.emplace(key, std::make_unique<Entity>(std::move(*entity_opt)));
  refcount[key] = 1;

  return EntityHandle<Entity>(manager, iter->second.get(),
                              [save_fn, release_fn, key](const Entity& e) {
                                save_fn(e);
                                release_fn(key);
                              });
}

template <typename Entity, typename Key, typename FindFn>
const Entity*
peek_entity_impl(Key key,
                 std::unordered_map<Key, std::unique_ptr<Entity>>& cache,
                 std::unordered_map<Key, int>& refcount, FindFn find_fn) {
  // Check if already cached
  auto it = cache.find(key);
  if (it != cache.end()) {
    refcount[key]++;  // Increment refcount to keep entity alive
    return it->second.get();
  }

  // Load from repository if not cached
  auto entity_opt = find_fn(key);
  if (!entity_opt) {
    return nullptr;
  }

  // Cache the entity and set initial refcount
  auto [iter, inserted] =
      cache.emplace(key, std::make_unique<Entity>(std::move(*entity_opt)));
  refcount[key] = 1;  // peek increments refcount like get does
  return iter->second.get();
}

template <typename Entity, typename Key>
void release_entity_impl(
    Key key, std::unordered_map<Key, std::unique_ptr<Entity>>& cache,
    std::unordered_map<Key, int>& refcount) {
  // Assumes lock is already held by caller
  auto it = refcount.find(key);
  if (it != refcount.end()) {
    it->second--;
    if (it->second <= 0) {
      cache.erase(key);
      refcount.erase(it);
    }
  }
}

template <typename Entity, typename Key>
void clear_cache_impl(std::unordered_map<Key, std::unique_ptr<Entity>>& cache,
                      std::unordered_map<Key, int>& refcount) {
  // Remove entities with no active handles (refcount == 0)
  // Keep entities with active handles to avoid breaking those handles
  for (auto it = cache.begin(); it != cache.end();) {
    if (refcount[it->first] == 0) {
      refcount.erase(it->first);
      it = cache.erase(it);
    } else {
      ++it;
    }
  }
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

EntityManager::EntityManager(Database& database)
    : db(database), store(database), races(store), ships(store), planets(store),
      stars(store), sectors(store), commods(store), blocks(store),
      powers(store), universe_repo(store) {}

// Race entity methods
EntityHandle<Race> EntityManager::get_race(player_t player) {
  return get_entity_impl<Race>(
      this, player, race_cache, race_refcount,
      [this](player_t p) { return races.find_by_player(p); },
      [this](const Race& r) { races.save(r); },
      [this](player_t p) { release_race(p); });
}

const Race* EntityManager::peek_race(player_t player) {
  return peek_entity_impl<Race>(
      player, race_cache, race_refcount,
      [this](player_t p) { return races.find_by_player(p); });
}

void EntityManager::release_race(player_t player) {
  release_entity_impl<Race>(player, race_cache, race_refcount);
}

// Ship entity methods
EntityHandle<Ship> EntityManager::get_ship(shipnum_t num) {
  return get_entity_impl<Ship>(
      this, num, ship_cache, ship_refcount,
      [this](shipnum_t n) { return ships.find_by_number(n); },
      [this](const Ship& s) { ships.save(s); },
      [this](shipnum_t n) { release_ship(n); });
}

const Ship* EntityManager::peek_ship(shipnum_t num) {
  return peek_entity_impl<Ship>(
      num, ship_cache, ship_refcount,
      [this](shipnum_t n) { return ships.find_by_number(n); });
}

void EntityManager::release_ship(shipnum_t num) {
  release_entity_impl<Ship>(num, ship_cache, ship_refcount);
}

EntityHandle<Ship> EntityManager::create_ship(const ship_struct& init_data) {
  // Get next available ship number
  shipnum_t num = ships.next_ship_number();

  // Create ship_struct, copying from provided data but overriding number
  ship_struct data = init_data;
  data.number = num;
  Ship new_ship{data};

  // Save immediately to database
  ships.save(new_ship);

  // Cache it
  auto [iter, inserted] =
      ship_cache.emplace(num, std::make_unique<Ship>(std::move(new_ship)));
  ship_refcount[num] = 1;

  // Save the new ship to database immediately
  ships.save(*iter->second);

  return {this, iter->second.get(), [this, num](const Ship& s) {
            ships.save(s);
            release_ship(num);
          }};
}

void EntityManager::delete_ship(shipnum_t num) {
  // Remove from cache if present
  ship_cache.erase(num);
  ship_refcount.erase(num);

  // Remove from database
  ships.delete_ship(num);
}

void EntityManager::delete_commod(int id) {
  // Remove from cache if present
  commod_cache.erase(id);
  commod_refcount.erase(id);

  // Remove from database
  commods.delete_commod(id);
}

EntityHandle<Commod> EntityManager::create_commod(const Commod& init_data) {
  // Get next available commod ID
  int id = commods.next_available_id();

  // Create Commod, copying from provided data but overriding id
  Commod new_commod = init_data;
  new_commod.id = id;

  // Save through repository (DAL)
  commods.save(new_commod);

  // Cache it
  auto [iter, inserted] =
      commod_cache.emplace(id, std::make_unique<Commod>(new_commod));
  commod_refcount[id] = 1;

  return {this, iter->second.get(), [this, id](const Commod& c) {
            commods.save(c);
            release_commod(id);
          }};
}

// Planet entity methods
EntityHandle<Planet> EntityManager::get_planet(starnum_t star,
                                               planetnum_t pnum) {
  auto key = std::make_pair(star, pnum);

  // Check if already cached
  auto it = planet_cache.find(key);
  if (it != planet_cache.end()) {
    planet_refcount[key]++;
    return {this, it->second.get(), [this, star, pnum](const Planet& p) {
              planets.save(p);
              release_planet(star, pnum);
            }};
  }

  // Load from repository
  auto planet_opt = planets.find_by_location(star, pnum);
  if (!planet_opt) {
    return {this, nullptr, [](const Planet&) {}};
  }

  // Cache the entity
  auto [iter, inserted] = planet_cache.emplace(
      key, std::make_unique<Planet>(std::move(*planet_opt)));
  planet_refcount[key] = 1;

  return {this, iter->second.get(), [this, star, pnum](const Planet& p) {
            planets.save(p);
            release_planet(star, pnum);
          }};
}

const Planet* EntityManager::peek_planet(starnum_t star, planetnum_t pnum) {
  auto key = std::make_pair(star, pnum);
  const auto* planet = peek_entity_impl<Planet>(
      key, planet_cache, planet_refcount, [this, star, pnum](auto) {
        return planets.find_by_location(star, pnum);
      });
  if (!planet) {
    throw EntityNotFoundError(
        std::format("Planet not found: star_id={}, planet_id={}", star, pnum));
  }
  return planet;
}

void EntityManager::release_planet(starnum_t star, planetnum_t pnum) {
  auto key = std::make_pair(star, pnum);
  auto it = planet_refcount.find(key);
  if (it != planet_refcount.end()) {
    it->second--;
    if (it->second <= 0) {
      planet_cache.erase(key);
      planet_refcount.erase(it);
    }
  }
}

// Star entity methods
EntityHandle<Star> EntityManager::get_star(starnum_t num) {
  return get_entity_impl<Star>(
      this, num, star_cache, star_refcount,
      [this](starnum_t n) { return stars.find_by_number(n); },
      [this](const Star& s) { stars.save(s); },
      [this](starnum_t n) { release_star(n); });
}

const Star* EntityManager::peek_star(starnum_t num) {
  const auto* star = peek_entity_impl<Star>(
      num, star_cache, star_refcount,
      [this](starnum_t n) { return stars.find_by_number(n); });
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
  return get_entity_impl<Commod>(
      this, id, commod_cache, commod_refcount,
      [this](int i) { return commods.find_by_id(i); },
      [this](const Commod& c) { commods.save(c); },
      [this](int i) { release_commod(i); });
}

const Commod* EntityManager::peek_commod(int id) {
  return peek_entity_impl<Commod>(
      id, commod_cache, commod_refcount,
      [this](int i) { return commods.find_by_id(i); });
}

void EntityManager::release_commod(int id) {
  release_entity_impl<Commod>(id, commod_cache, commod_refcount);
}

// Block entity methods
EntityHandle<block> EntityManager::get_block(int id) {
  return get_entity_impl<block>(
      this, id, block_cache, block_refcount,
      [this](int i) { return blocks.find_by_id(i); },
      [this](const block& b) { blocks.save(b); },
      [this](int i) { release_block(i); });
}

const block* EntityManager::peek_block(int id) {
  return peek_entity_impl<block>(
      id, block_cache, block_refcount,
      [this](int i) { return blocks.find_by_id(i); });
}

void EntityManager::release_block(int id) {
  release_entity_impl<block>(id, block_cache, block_refcount);
}

// Power entity methods
EntityHandle<power> EntityManager::get_power(int id) {
  return get_entity_impl<power>(
      this, id, power_cache, power_refcount,
      [this](int i) { return powers.find_by_id(i); },
      [this](const power& p) { powers.save(p); },
      [this](int i) { release_power(i); });
}

const power* EntityManager::peek_power(int id) {
  return peek_entity_impl<power>(
      id, power_cache, power_refcount,
      [this](int i) { return powers.find_by_id(i); });
}

void EntityManager::release_power(int id) {
  release_entity_impl<power>(id, power_cache, power_refcount);
}

// Universe entity methods (singleton)
EntityHandle<universe_struct> EntityManager::get_universe() {
  if (global_universe_cache) {
    global_universe_refcount++;
    return {this, global_universe_cache.get(),
            [this](const universe_struct& sd) {
              universe_repo.save(sd);
              release_universe();
            }};
  }

  auto universe_opt = universe_repo.get_global_data();
  if (!universe_opt) {
    return {this, nullptr, [](const universe_struct&) {}};
  }

  global_universe_cache = std::make_unique<universe_struct>(*universe_opt);
  global_universe_refcount = 1;

  return {this, global_universe_cache.get(), [this](const universe_struct& sd) {
            universe_repo.save(sd);
            release_universe();
          }};
}

const universe_struct* EntityManager::peek_universe() {
  // Check if already cached
  if (global_universe_cache) {
    return global_universe_cache.get();
  }

  // Load from repository if not cached
  auto universe_opt = universe_repo.get_global_data();
  if (!universe_opt) {
    return nullptr;
  }

  // Cache the entity (but don't increment refcount - this is read-only)
  global_universe_cache = std::make_unique<universe_struct>(*universe_opt);
  return global_universe_cache.get();
}

void EntityManager::release_universe() {
  global_universe_refcount--;
  if (global_universe_refcount <= 0) {
    global_universe_cache.reset();
    global_universe_refcount = 0;
  }
}

// Query methods
int EntityManager::num_commods() {
  // Count commods by listing all IDs in the database
  return store.list_ids("tbl_commod").size();
}

int EntityManager::next_available_commod_id() {
  CommodRepository commod_repo(store);
  return commod_repo.next_available_id();
}

player_t EntityManager::num_races() {
  // Count races by listing all IDs in the database
  return store.list_ids("tbl_race").size();
}

shipnum_t EntityManager::num_ships() {
  // Count ships by listing all IDs in the database
  return store.list_ids("tbl_ship").size();
}

// Utility methods
void EntityManager::flush_all() {
  // Save all cached entities - entities now contain their own IDs
  flush_cache_impl<Race>(race_cache, [this](const Race& r) { races.save(r); });
  flush_cache_impl<Ship>(ship_cache, [this](const Ship& s) { ships.save(s); });
  flush_cache_impl<Planet>(planet_cache,
                           [this](const Planet& p) { planets.save(p); });
  flush_cache_impl<Star>(star_cache, [this](const Star& s) { stars.save(s); });
  flush_cache_impl<SectorMap>(
      sectormap_cache, [this](const SectorMap& sm) { sectors.save_map(sm); });
  flush_cache_impl<Commod>(commod_cache,
                           [this](const Commod& c) { commods.save(c); });
  flush_cache_impl<block>(block_cache,
                          [this](const block& b) { blocks.save(b); });
  flush_cache_impl<power>(power_cache,
                          [this](const power& p) { powers.save(p); });

  if (global_universe_cache) {
    universe_repo.save(*global_universe_cache);
  }
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
  clear_cache_impl<block>(block_cache, block_refcount);
  clear_cache_impl<power>(power_cache, power_refcount);

  // Clear global universe_struct if no active handles
  if (global_universe_refcount == 0) {
    global_universe_cache.reset();
  }
}

// Business logic operations
std::optional<player_t>
EntityManager::find_player_by_name(const std::string& name) {
  player_t rnum = 0;

  if (name.empty()) return std::nullopt;

  if (isdigit(name[0])) {
    if ((rnum = std::stoi(name)) < 1 || rnum > num_races()) return std::nullopt;
    return rnum;
  }

  // Iterate through all races using peek_race
  for (player_t p = 1; p <= num_races(); p++) {
    const auto* race = peek_race(p);
    if (race && name == race->name) {
      return race->Playernum;
    }
  }
  return std::nullopt;
}

void EntityManager::kill_ship(player_t Playernum, Ship& ship) {
  if (std::holds_alternative<MindData>(ship.special())) {
    auto mind = std::get<MindData>(ship.special());
    mind.who_killed = Playernum;
    ship.special() = mind;
  }
  ship.alive() = 0;
  ship.notified() = 0; /* prepare the ship for recycling */

  if (ship.type() != ShipType::STYPE_POD &&
      ship.type() != ShipType::OTYPE_FACTORY) {
    /* pods don't do things to morale, ditto for factories */
    auto victim_handle = get_race(ship.owner());
    if (!victim_handle.get()) {
      std::cerr << "Database corruption, race not found.";
      std::abort();
    }
    auto& victim = *victim_handle;
    if (victim.Gov_ship == ship.number()) victim.Gov_ship = 0;

    if (!victim.God && Playernum != ship.owner() &&
        ship.type() != ShipType::OTYPE_VN) {
      auto killer_handle = get_race(Playernum);
      if (!killer_handle.get()) {
        std::cerr << "Database corruption, race not found.";
        std::abort();
      }
      auto& killer = *killer_handle;
      adjust_morale(killer, victim, (int)ship.build_cost());
      // Both killer and victim auto-save when handles go out of scope
    } else if (ship.owner() == Playernum && !ship.docked() && max_crew(ship)) {
      victim.morale -= 2L * ship.build_cost(); /* scuttle/scrap */
    }
    // victim auto-saves when handle goes out of scope
  }

  if (ship.type() == ShipType::OTYPE_VN ||
      ship.type() == ShipType::OTYPE_BERS) {
    auto sdata_handle = get_universe();
    if (!sdata_handle.get()) {
      std::cerr << "Database corruption, universe_struct not found.";
      std::abort();
    }
    auto& Sdata = *sdata_handle;

    /* add ship to VN shit list */
    if (std::holds_alternative<MindData>(ship.special())) {
      auto mind = std::get<MindData>(ship.special());
      Sdata.VN_hitlist[mind.who_killed - 1] += 1;
    }

    /* keep track of where these VN's were shot up */
    if (Sdata.VN_index1[Playernum - 1] == -1)
      /* there's no star in the first index */
      Sdata.VN_index1[Playernum - 1] = ship.storbits();
    else if (Sdata.VN_index2[Playernum - 1] == -1)
      /* there's no star in the second index */
      Sdata.VN_index2[Playernum - 1] = ship.storbits();
    else {
      /* pick an index to supplant */
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<int> dis(0, 1);
      if (dis(gen))
        Sdata.VN_index1[Playernum - 1] = ship.storbits();
      else
        Sdata.VN_index2[Playernum - 1] = ship.storbits();
    }
    // Sdata auto-saves when handle goes out of scope
  }

  if (ship.type() == ShipType::OTYPE_TOXWC &&
      ship.whatorbits() == ScopeLevel::LEVEL_PLAN) {
    auto planet_handle = get_planet(ship.storbits(), ship.pnumorbits());
    if (!planet_handle.get()) {
      std::cerr << "Database corruption, planet not found.";
      std::abort();
    }
    auto& planet = *planet_handle;
    if (std::holds_alternative<WasteData>(ship.special())) {
      auto waste = std::get<WasteData>(ship.special());
      planet.conditions(TOXIC) =
          MIN(100, planet.conditions(TOXIC) + waste.toxic);
    }
    // planet auto-saves when handle goes out of scope
  }

  /* undock the stuff docked with it */
  if (ship.docked() && ship.whatorbits() != ScopeLevel::LEVEL_SHIP &&
      ship.whatdest() == ScopeLevel::LEVEL_SHIP) {
    auto dest_ship_handle = get_ship(ship.destshipno());
    if (!dest_ship_handle.get()) {
      std::cerr << "Database corruption, ship not found.";
      std::abort();
    }
    auto& s = *dest_ship_handle;
    s.docked() = 0;
    s.whatdest() = ScopeLevel::LEVEL_UNIV;
    // s auto-saves when handle goes out of scope
  }

  /* landed ships are killed */
  ShipList shiplist(*this, ship.ships());
  for (auto ship_handle : shiplist) {
    Ship& s = *ship_handle;   // Get mutable reference
    kill_ship(Playernum, s);  // Recursive call to member function
  }
}

// SectorMap operations (cached with RAII like other entities)
EntityHandle<SectorMap> EntityManager::get_sectormap(starnum_t star,
                                                     planetnum_t pnum) {
  auto key = std::make_pair(star, pnum);

  // Check if already cached
  auto it = sectormap_cache.find(key);
  if (it != sectormap_cache.end()) {
    sectormap_refcount[key]++;
    return {this, it->second.get(), [this, star, pnum](const SectorMap& sm) {
              sectors.save_map(sm);
              release_sectormap(star, pnum);
            }};
  }

  // Need to load from repository - but we need the Planet to construct
  // SectorMap
  auto planet_opt = planets.find_by_location(star, pnum);
  if (!planet_opt) {
    return {this, nullptr, [](const SectorMap&) {}};
  }

  // Load the sector map
  SectorMap loaded_map = sectors.load_map(*planet_opt);

  // Cache the entity
  auto [iter, inserted] = sectormap_cache.emplace(
      key, std::make_unique<SectorMap>(std::move(loaded_map)));
  sectormap_refcount[key] = 1;

  return {this, iter->second.get(), [this, star, pnum](const SectorMap& sm) {
            sectors.save_map(sm);
            release_sectormap(star, pnum);
          }};
}

const SectorMap* EntityManager::peek_sectormap(starnum_t star,
                                               planetnum_t pnum) {
  auto key = std::make_pair(star, pnum);
  const auto* sectormap = peek_entity_impl<SectorMap>(
      key, sectormap_cache, sectormap_refcount, [this, star, pnum](auto) {
        auto planet_opt = planets.find_by_location(star, pnum);
        if (!planet_opt) return std::optional<SectorMap>{};
        return std::optional<SectorMap>(sectors.load_map(*planet_opt));
      });
  if (!sectormap) {
    throw EntityNotFoundError(std::format(
        "SectorMap not found: star_id={}, planet_id={}", star, pnum));
  }
  return sectormap;
}

void EntityManager::release_sectormap(starnum_t star, planetnum_t pnum) {
  auto key = std::make_pair(star, pnum);
  auto it = sectormap_refcount.find(key);
  if (it != sectormap_refcount.end()) {
    it->second--;
    if (it->second <= 0) {
      sectormap_cache.erase(key);
      sectormap_refcount.erase(it);
    }
  }
}
