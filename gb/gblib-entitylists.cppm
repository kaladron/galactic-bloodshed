// SPDX-License-Identifier: Apache-2.0

/**
 * @file gblib-entitylists.cppm
 * @brief Entity list iterators for type-safe, RAII-based iteration over
 *        game entities (races, stars, planets, ships).
 *
 * These classes provide a clean iteration interface over EntityManager
 * entities with automatic save-on-scope-exit semantics through EntityHandle.
 *
 * Usage:
 *   // Iterate over all races with RAII auto-save
 *   for (auto race_handle : RaceList(entity_manager)) {
 *     race_handle->tech += 1.0;  // Marks dirty, will auto-save
 *   }
 *
 *   // Iterate over all stars
 *   for (auto star_handle : StarList(entity_manager)) {
 *     process_star(*star_handle);
 *   }
 *
 *   // Iterate over planets of a specific star
 *   const Star& star = ...;
 *   for (auto planet_handle : PlanetList(entity_manager, starnum, star)) {
 *     process_planet(*planet_handle);
 *   }
 *
 *   // Iterate over ships (multiple modes available)
 *   for (auto ship_handle : ShipList(entity_manager,
 * ShipList::IterationType::AllAlive)) { ship_handle->fuel += 10;  // Marks
 * dirty, will auto-save
 *   }
 */

export module gblib:entitylists;

import std;

import :gameobj;
import :services;
import :types;
import :race;
import :star;
import :planet;
import :ships;

/**
 * Iterator class for races (1-indexed, 1..num_races).
 * Returns EntityHandle<Race> for RAII auto-save behavior.
 *
 * Skips any race slots that don't exist (sparse iteration).
 */
export class RaceList {
public:
  explicit RaceList(EntityManager& em) : em_(&em), count_(em.num_races()) {}

  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = EntityHandle<Race>;
    using difference_type = std::ptrdiff_t;

    Iterator(EntityManager* em, player_t current, player_t end)
        : em_(em), current_(current), end_(end) {
      advance_to_valid();
    }

    EntityHandle<Race> operator*() {
      return em_->get_race(current_);
    }

    Iterator& operator++() {
      ++current_;
      advance_to_valid();
      return *this;
    }

    bool operator!=(const Iterator& other) const {
      return current_ != other.current_;
    }

    bool operator==(const Iterator& other) const {
      return current_ == other.current_;
    }

  private:
    void advance_to_valid() {
      while (current_ <= end_ && !em_->peek_race(current_)) {
        ++current_;
      }
    }

    EntityManager* em_;
    player_t current_;
    player_t end_;
  };

  Iterator begin() {
    return Iterator(em_, 1, count_);
  }
  Iterator end() {
    return Iterator(em_, count_.value + 1, count_);
  }

private:
  EntityManager* em_;
  player_t count_;
};

/**
 * Iterator class for stars (0-indexed, 0..numstars-1).
 * Returns EntityHandle<Star> for RAII auto-save behavior.
 */
export class StarList {
public:
  explicit StarList(EntityManager& em)
      : em_(&em), count_(em.peek_universe()->numstars) {}

  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = EntityHandle<Star>;
    using difference_type = std::ptrdiff_t;

    Iterator(EntityManager* em, starnum_t current,
             [[maybe_unused]] starnum_t end)
        : em_(em), current_(current) {}

    EntityHandle<Star> operator*() {
      return em_->get_star(current_);
    }

    Iterator& operator++() {
      ++current_;
      return *this;
    }

    bool operator!=(const Iterator& other) const {
      return current_ != other.current_;
    }

    bool operator==(const Iterator& other) const {
      return current_ == other.current_;
    }

  private:
    EntityManager* em_;
    starnum_t current_;
  };

  Iterator begin() {
    return Iterator(em_, 0, count_);
  }
  Iterator end() {
    return Iterator(em_, count_, count_);
  }

private:
  EntityManager* em_;
  starnum_t count_;
};

/**
 * Iterator class for planets of a star (0-indexed, 0..numplanets-1).
 * Returns EntityHandle<Planet> for RAII auto-save behavior.
 */
export class PlanetList {
public:
  PlanetList(EntityManager& em, starnum_t star, planetnum_t numplanets)
      : em_(&em), star_(star), count_(numplanets) {}

  PlanetList(EntityManager& em, starnum_t star, const Star& star_data)
      : em_(&em), star_(star), count_(star_data.numplanets()) {}

  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = EntityHandle<Planet>;
    using difference_type = std::ptrdiff_t;

    Iterator(EntityManager* em, starnum_t star, planetnum_t current,
             [[maybe_unused]] planetnum_t end)
        : em_(em), star_(star), current_(current) {}

    EntityHandle<Planet> operator*() {
      return em_->get_planet(star_, current_);
    }

    Iterator& operator++() {
      ++current_;
      return *this;
    }

    bool operator!=(const Iterator& other) const {
      return current_ != other.current_;
    }

    bool operator==(const Iterator& other) const {
      return current_ == other.current_;
    }

  private:
    EntityManager* em_;
    starnum_t star_;
    planetnum_t current_;
  };

  Iterator begin() {
    return Iterator(em_, star_, 0, count_);
  }
  Iterator end() {
    return Iterator(em_, star_, count_, count_);
  }

private:
  EntityManager* em_;
  starnum_t star_;
  planetnum_t count_;
};

/**
 * Iterator class for commodities (1-indexed, 1..num_commods).
 * Returns EntityHandle<Commod> for RAII auto-save behavior.
 * Only returns valid commodities (non-null, has owner, has amount).
 */
export class CommodList {
public:
  explicit CommodList(EntityManager& em) : em_(&em), count_(em.num_commods()) {}

  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = EntityHandle<Commod>;
    using difference_type = std::ptrdiff_t;

    Iterator(EntityManager* em, int current, int end)
        : em_(em), current_(current), end_(end) {
      advance_to_valid();
    }

    EntityHandle<Commod> operator*() {
      return em_->get_commod(current_);
    }

    Iterator& operator++() {
      ++current_;
      advance_to_valid();
      return *this;
    }

    bool operator!=(const Iterator& other) const {
      return current_ != other.current_;
    }

    bool operator==(const Iterator& other) const {
      return current_ == other.current_;
    }

  private:
    void advance_to_valid() {
      while (current_ <= end_) {
        const auto* c = em_->peek_commod(current_);
        if (c && c->owner.value && c->amount) {
          return;  // Found valid commodity
        }
        ++current_;
      }
    }

    EntityManager* em_;
    int current_;
    int end_;
  };

  Iterator begin() {
    return {em_, 1, count_};
  }
  Iterator end() {
    return {em_, count_ + 1, count_};
  }

private:
  EntityManager* em_;
  int count_;
};

// ============================================================================
// Ship iteration classes
// ============================================================================

/**
 * RAII wrapper for mutable ship access.
 * Automatically saves changes when the wrapper goes out of scope.
 * This type-erased wrapper avoids circular module dependencies.
 */
export class ShipHandle {
public:
  // Constructor - implemented in .cc to avoid circular dependency
  explicit ShipHandle(EntityHandle<Ship>&& handle);
  ~ShipHandle();

  // Delete copy, allow move
  ShipHandle(const ShipHandle&) = delete;
  ShipHandle& operator=(const ShipHandle&) = delete;
  ShipHandle(ShipHandle&&) noexcept;
  ShipHandle& operator=(ShipHandle&&) noexcept;

  // Access operators
  Ship& operator*();
  const Ship& operator*() const;
  Ship* operator->();
  const Ship* operator->() const;
  Ship* get();
  const Ship* get() const;

  // Explicit read-only access (doesn't mark dirty)
  const Ship& peek() const;

  // Force save without waiting for destructor
  void save();

private:
  struct Impl;  // Pimpl to hide EntityHandle<Ship>
  std::unique_ptr<Impl> pimpl;
};

/**
 * Modern ShipList class with EntityManager integration and RAII support.
 * Provides automatic save-on-scope-exit semantics through ShipHandle.
 *
 * Usage:
 *   // Read-only iteration (const ShipList)
 *   const ShipList ships(g.entity_manager, ship.ships);
 *   for (const Ship* ship : ships) {
 *     g.out << ship->name << "\n";  // Read-only, no modifications allowed
 *   }
 *
 *   // Mutable iteration (auto-saves on scope exit)
 *   ShipList ships(g.entity_manager, ship.ships);
 *   for (auto ship_handle : ships) {
 *     auto& s = *ship_handle;
 *     s.fuel += 10;  // Marks dirty, will auto-save
 *   }
 */
export class ShipList {
public:
  enum class IterationType {
    Nested,   ///< Follow ship.ships linked list
    Scope,    ///< All ships at current scope (universe/star/planet/ship)
    All,      ///< All ships in game (1..num_ships), including dead
    AllAlive  ///< All alive ships in game (1..num_ships)
  };

  // Mutable version - returns ShipHandle
  ShipList(EntityManager& em, shipnum_t start,
           IterationType type = IterationType::Nested);
  ShipList(EntityManager& em, const GameObj& g,
           IterationType type = IterationType::Scope);
  // Constructor for All/AllAlive iteration (all ships in game)
  ShipList(EntityManager& em, IterationType type);

  // Forward declaration for iterators
  class MutableIterator;
  class ConstIterator;

  // Iterator support - mutable version
  MutableIterator begin();
  MutableIterator end();

  // Iterator support - const version (read-only)
  ConstIterator begin() const;
  ConstIterator end() const;
  ConstIterator cbegin() const;
  ConstIterator cend() const;

private:
  mutable EntityManager* em{nullptr};
  shipnum_t start_ship{0};
  IterationType iteration_type;
  ScopeLevel scope_level{ScopeLevel::LEVEL_UNIV};
  starnum_t snum{0};
  planetnum_t pnum{0};
  player_t player{0};

  // Helper to check if a ship matches the current scope
  bool matches_scope(const Ship& ship) const;
};

// Iterator classes
/**
 * Iterator for mutable ship access - returns ShipHandle
 * Provides RAII with automatic save-on-scope-exit.
 */
class ShipList::MutableIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = ShipHandle;
  using difference_type = std::ptrdiff_t;

  MutableIterator(EntityManager& em, shipnum_t current, IterationType type,
                  ScopeLevel level, starnum_t snum, planetnum_t pnum,
                  player_t player);

  MutableIterator& operator++();
  ShipHandle operator*();  // Defined in .cc file
  bool operator==(const MutableIterator& other) const;
  bool operator!=(const MutableIterator& other) const;

private:
  EntityManager& em;
  shipnum_t current;
  IterationType type;
  ScopeLevel scope_level;
  starnum_t snum;
  planetnum_t pnum;
  player_t player;

  void advance_to_next_match();
  bool matches_scope(const Ship& ship) const;
};

/**
 * Iterator for const (read-only) ship access - returns const Ship*
 * Uses peek_ship() to avoid marking entities as dirty.
 * Provides compile-time guarantees against modification.
 */
class ShipList::ConstIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = const Ship*;
  using difference_type = std::ptrdiff_t;
  using pointer = const Ship**;
  using reference = const Ship*&;

  ConstIterator(EntityManager& em, shipnum_t current, IterationType type,
                ScopeLevel level, starnum_t snum, planetnum_t pnum,
                player_t player);

  ConstIterator& operator++();
  const Ship* operator*() const;
  bool operator==(const ConstIterator& other) const;
  bool operator!=(const ConstIterator& other) const;

private:
  EntityManager& em;
  shipnum_t current;
  IterationType type;
  ScopeLevel scope_level;
  starnum_t snum;
  planetnum_t pnum;
  player_t player;

  void advance_to_next_match();
  bool matches_scope(const Ship& ship) const;
};
