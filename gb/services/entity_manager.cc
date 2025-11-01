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
EntityHandle<Entity> get_entity_impl(
    EntityManager* manager, Key key,
    std::unordered_map<Key, std::unique_ptr<Entity>>& cache,
    std::unordered_map<Key, int>& refcount, std::mutex& cache_mutex,
    FindFn find_fn, SaveFn save_fn, ReleaseFn release_fn) {
  std::lock_guard lock(cache_mutex);

  // Check if already cached
  auto it = cache.find(key);
  if (it != cache.end()) {
    refcount[key]++;
    return {manager, it->second.get(),
            [&cache_mutex, save_fn, release_fn, key](const Entity& e) {
              std::lock_guard lock(cache_mutex);
              save_fn(e, key);
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

  return EntityHandle<Entity>(
      manager, iter->second.get(),
      [&cache_mutex, save_fn, release_fn, key](const Entity& e) {
        std::lock_guard lock(cache_mutex);
        save_fn(e, key);
        release_fn(key);
      });
}

template <typename Entity, typename Key>
const Entity* peek_entity_impl(
    Key key, std::unordered_map<Key, std::unique_ptr<Entity>>& cache,
    std::mutex& cache_mutex) {
  std::lock_guard lock(cache_mutex);
  auto it = cache.find(key);
  if (it != cache.end()) {
    return it->second.get();
  }
  return nullptr;
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
    save_fn(key, *entity);
  }
}
}  // namespace

EntityManager::EntityManager(Database& database)
    : db(database),
      store(database),
      races(store),
      ships(store),
      planets(store),
      stars(store),
      sectors(store),
      commods(store),
      blocks(store),
      powers(store),
      stardata_repo(store) {}

// Race entity methods
EntityHandle<Race> EntityManager::get_race(player_t player) {
  return get_entity_impl<Race>(
      this, player, race_cache, race_refcount, cache_mutex,
      [this](player_t p) { return races.find_by_player(p); },
      [this](const Race& r, player_t) { races.save_race(r); },
      [this](player_t p) { release_race(p); });
}

const Race* EntityManager::peek_race(player_t player) {
  return peek_entity_impl<Race>(player, race_cache, cache_mutex);
}

void EntityManager::release_race(player_t player) {
  release_entity_impl<Race>(player, race_cache, race_refcount);
}

// Ship entity methods
EntityHandle<Ship> EntityManager::get_ship(shipnum_t num) {
  return get_entity_impl<Ship>(
      this, num, ship_cache, ship_refcount, cache_mutex,
      [this](shipnum_t n) { return ships.find_by_number(n); },
      [this](const Ship& s, shipnum_t) { ships.save_ship(s); },
      [this](shipnum_t n) { release_ship(n); });
}

const Ship* EntityManager::peek_ship(shipnum_t num) {
  return peek_entity_impl<Ship>(num, ship_cache, cache_mutex);
}

void EntityManager::release_ship(shipnum_t num) {
  release_entity_impl<Ship>(num, ship_cache, ship_refcount);
}

EntityHandle<Ship> EntityManager::create_ship() {
  std::lock_guard lock(cache_mutex);

  // Get next available ship number
  shipnum_t num = ships.next_ship_number();

  // Create new ship
  Ship new_ship{};
  new_ship.number = num;

  // Cache it
  auto [iter, inserted] =
      ship_cache.emplace(num, std::make_unique<Ship>(new_ship));
  ship_refcount[num] = 1;

  return {this, iter->second.get(), [this, num](const Ship& s) {
            std::lock_guard lock(cache_mutex);
            ships.save_ship(s);
            release_ship(num);
          }};
}

void EntityManager::delete_ship(shipnum_t num) {
  std::lock_guard lock(cache_mutex);

  // Remove from cache if present
  ship_cache.erase(num);
  ship_refcount.erase(num);

  // Remove from database
  ships.delete_ship(num);
}

// Planet entity methods
EntityHandle<Planet> EntityManager::get_planet(starnum_t star,
                                               planetnum_t pnum) {
  std::lock_guard lock(cache_mutex);

  auto key = std::make_pair(star, pnum);

  // Check if already cached
  auto it = planet_cache.find(key);
  if (it != planet_cache.end()) {
    planet_refcount[key]++;
    return {this, it->second.get(), [this, star, pnum](const Planet& p) {
              std::lock_guard lock(cache_mutex);
              planets.save_planet(p);
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
            std::lock_guard lock(cache_mutex);
            planets.save_planet(p);
            release_planet(star, pnum);
          }};
}

const Planet* EntityManager::peek_planet(starnum_t star, planetnum_t pnum) {
  std::lock_guard lock(cache_mutex);

  auto it = planet_cache.find(std::make_pair(star, pnum));
  if (it != planet_cache.end()) {
    return it->second.get();
  }
  return nullptr;
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
EntityHandle<star_struct> EntityManager::get_star(starnum_t num) {
  return get_entity_impl<star_struct>(
      this, num, star_cache, star_refcount, cache_mutex,
      [this](starnum_t n) { return stars.find_by_number(n); },
      [this](const star_struct& s, starnum_t) { stars.save_star(s); },
      [this](starnum_t n) { release_star(n); });
}

const star_struct* EntityManager::peek_star(starnum_t num) {
  return peek_entity_impl<star_struct>(num, star_cache, cache_mutex);
}

void EntityManager::release_star(starnum_t num) {
  release_entity_impl<star_struct>(num, star_cache, star_refcount);
}

// Commod entity methods
EntityHandle<Commod> EntityManager::get_commod(int id) {
  return get_entity_impl<Commod>(
      this, id, commod_cache, commod_refcount, cache_mutex,
      [this](int i) { return commods.find_by_id(i); },
      [this](const Commod& c, int) { commods.save_commod(c); },
      [this](int i) { release_commod(i); });
}

void EntityManager::release_commod(int id) {
  release_entity_impl<Commod>(id, commod_cache, commod_refcount);
}

// Block entity methods
EntityHandle<block> EntityManager::get_block(int id) {
  return get_entity_impl<block>(
      this, id, block_cache, block_refcount, cache_mutex,
      [this](int i) { return blocks.find_by_id(i); },
      [this](const block& b, int) { blocks.save_block(b); },
      [this](int i) { release_block(i); });
}

void EntityManager::release_block(int id) {
  release_entity_impl<block>(id, block_cache, block_refcount);
}

// Power entity methods
EntityHandle<power> EntityManager::get_power(int id) {
  return get_entity_impl<power>(
      this, id, power_cache, power_refcount, cache_mutex,
      [this](int i) { return powers.find_by_id(i); },
      [this](const power& p, int) { powers.save_power(p); },
      [this](int i) { release_power(i); });
}

void EntityManager::release_power(int id) {
  release_entity_impl<power>(id, power_cache, power_refcount);
}

// Stardata entity methods (singleton)
EntityHandle<stardata> EntityManager::get_stardata() {
  std::lock_guard lock(cache_mutex);

  if (global_stardata_cache) {
    global_stardata_refcount++;
    return {this, global_stardata_cache.get(), [this](const stardata& sd) {
              std::lock_guard lock(cache_mutex);
              stardata_repo.save_global_data(sd);
              release_stardata();
            }};
  }

  auto stardata_opt = stardata_repo.get_global_data();
  if (!stardata_opt) {
    return {this, nullptr, [](const stardata&) {}};
  }

  global_stardata_cache = std::make_unique<stardata>(*stardata_opt);
  global_stardata_refcount = 1;

  return {this, global_stardata_cache.get(), [this](const stardata& sd) {
            std::lock_guard lock(cache_mutex);
            stardata_repo.save_global_data(sd);
            release_stardata();
          }};
}

void EntityManager::release_stardata() {
  global_stardata_refcount--;
  if (global_stardata_refcount <= 0) {
    global_stardata_cache.reset();
    global_stardata_refcount = 0;
  }
}

// Utility methods
void EntityManager::flush_all() {
  std::lock_guard lock(cache_mutex);

  // Save all cached entities - entities now contain their own IDs
  flush_cache_impl<Race>(
      race_cache, [this](player_t, const Race& r) { races.save_race(r); });
  flush_cache_impl<Ship>(ship_cache, [this](shipnum_t, const Ship& s) {
    ships.save_ship(s);
  });
  flush_cache_impl<Planet>(planet_cache,
                           [this](const auto&, const Planet& p) {
                             planets.save_planet(p);
                           });
  flush_cache_impl<star_struct>(
      star_cache,
      [this](starnum_t, const star_struct& s) { stars.save_star(s); });
  flush_cache_impl<Commod>(commod_cache, [this](int, const Commod& c) {
    commods.save_commod(c);
  });
  flush_cache_impl<block>(block_cache, [this](int, const block& b) {
    blocks.save_block(b);
  });
  flush_cache_impl<power>(power_cache, [this](int, const power& p) {
    powers.save_power(p);
  });

  if (global_stardata_cache) {
    stardata_repo.save_global_data(*global_stardata_cache);
  }
}

void EntityManager::clear_cache() {
  std::lock_guard lock(cache_mutex);

  // Clear entities that have no active handles (refcount == 0)
  // Keep entities with active handles to avoid breaking those handles
  clear_cache_impl<Race>(race_cache, race_refcount);
  clear_cache_impl<Ship>(ship_cache, ship_refcount);
  clear_cache_impl<Planet>(planet_cache, planet_refcount);
  clear_cache_impl<star_struct>(star_cache, star_refcount);
  clear_cache_impl<Commod>(commod_cache, commod_refcount);
  clear_cache_impl<block>(block_cache, block_refcount);
  clear_cache_impl<power>(power_cache, power_refcount);

  // Clear global stardata if no active handles
  if (global_stardata_refcount == 0) {
    global_stardata_cache.reset();
  }
}
