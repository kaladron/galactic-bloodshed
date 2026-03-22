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
 *   // Read-only iteration is explicit and cheap
 *   for (const Race* race : RaceList::readonly(entity_manager)) {
 *     observe(*race);
 *   }
 *
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
 *
 * Conventions:
 *   - Prefer `XxxList::readonly(...)` for read-only iteration.
 *   - Use `XxxList(...)` only when you need mutable RAII handles.
 *   - Keep explicit numeric loops only when caller logic needs stable indices
 *     for side arrays or bookkeeping.
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

export template <typename Entity>
struct EntityListTraits;

export template <typename Derived>
class ReadonlyFactory {
public:
  template <typename... Args>
  static const Derived readonly(Args&&... args) {
    return Derived(std::forward<Args>(args)...);
  }

protected:
  ReadonlyFactory() = default;
};

export template <>
struct EntityListTraits<Race> {
  using index_type = player_t;

  static index_type count(EntityManager& em) {
    return em.num_races();
  }

  static constexpr index_type first_index() {
    return index_type{1};
  }

  static index_type end_index(index_type count) {
    return index_type{count.value + 1};
  }

  static index_type next(index_type current) {
    ++current;
    return current;
  }

  static EntityHandle<Race> get(EntityManager& em, index_type index) {
    return em.get_race(index);
  }

  static const Race* peek(EntityManager& em, index_type index) {
    return em.peek_race(index);
  }

  static bool is_valid(const Race* entity) {
    return entity != nullptr;
  }
};

export template <>
struct EntityListTraits<Star> {
  using index_type = starnum_t;

  static index_type count(EntityManager& em) {
    return em.peek_universe()->numstars;
  }

  static constexpr index_type first_index() {
    return index_type{0};
  }

  static index_type end_index(index_type count) {
    return count;
  }

  static index_type next(index_type current) {
    ++current;
    return current;
  }

  static EntityHandle<Star> get(EntityManager& em, index_type index) {
    return em.get_star(index);
  }

  static const Star* peek(EntityManager& em, index_type index) {
    return em.peek_star(index);
  }

  static bool is_valid([[maybe_unused]] const Star* entity) {
    return true;
  }
};

export template <>
struct EntityListTraits<Commod> {
  using index_type = int;

  static index_type count(EntityManager& em) {
    return em.num_commods();
  }

  static constexpr index_type first_index() {
    return 1;
  }

  static index_type end_index(index_type count) {
    return count + 1;
  }

  static index_type next(index_type current) {
    return current + 1;
  }

  static EntityHandle<Commod> get(EntityManager& em, index_type index) {
    return em.get_commod(index);
  }

  static const Commod* peek(EntityManager& em, index_type index) {
    return em.peek_commod(index);
  }

  static bool is_valid(const Commod* entity) {
    return entity && entity->owner.value && entity->amount;
  }
};

export template <>
struct EntityListTraits<Planet> {
  using primary_key_type = starnum_t;
  using index_type = planetnum_t;

  static constexpr index_type first_index() {
    return index_type{0};
  }

  static index_type end_index(index_type count) {
    return count;
  }

  static index_type next(index_type current) {
    ++current;
    return current;
  }

  static EntityHandle<Planet> get(EntityManager& em, primary_key_type primary,
                                  index_type index) {
    return em.get_planet(primary, index);
  }

  static const Planet* peek(EntityManager& em, primary_key_type primary,
                            index_type index) {
    return em.peek_planet(primary, index);
  }

  static bool is_valid([[maybe_unused]] const Planet* entity) {
    return true;
  }
};

export template <typename Entity, typename Derived>
class SimpleEntityList : public ReadonlyFactory<Derived> {
public:
  using traits_type = EntityListTraits<Entity>;
  using index_type = typename traits_type::index_type;

protected:
  explicit SimpleEntityList(EntityManager& em)
      : em_(&em), count_(traits_type::count(em)) {}

public:
  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = EntityHandle<Entity>;
    using difference_type = std::ptrdiff_t;

    Iterator(EntityManager* em, index_type current, index_type end)
        : em_(em), current_(current), end_(end) {
      advance_to_valid();
    }

    value_type operator*() {
      return traits_type::get(*em_, current_);
    }

    Iterator& operator++() {
      current_ = traits_type::next(current_);
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
      while (current_ != end_ &&
             !traits_type::is_valid(traits_type::peek(*em_, current_))) {
        current_ = traits_type::next(current_);
      }
    }

    EntityManager* em_;
    index_type current_;
    index_type end_;
  };

  class ConstIterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const Entity*;
    using difference_type = std::ptrdiff_t;

    ConstIterator(EntityManager* em, index_type current, index_type end)
        : em_(em), current_(current), end_(end) {
      advance_to_valid();
    }

    const Entity* operator*() const {
      return traits_type::peek(*em_, current_);
    }

    ConstIterator& operator++() {
      current_ = traits_type::next(current_);
      advance_to_valid();
      return *this;
    }

    bool operator!=(const ConstIterator& other) const {
      return current_ != other.current_;
    }

    bool operator==(const ConstIterator& other) const {
      return current_ == other.current_;
    }

  private:
    void advance_to_valid() {
      while (current_ != end_ &&
             !traits_type::is_valid(traits_type::peek(*em_, current_))) {
        current_ = traits_type::next(current_);
      }
    }

    EntityManager* em_;
    index_type current_;
    index_type end_;
  };

  Iterator begin() {
    return {em_, traits_type::first_index(), traits_type::end_index(count_)};
  }

  Iterator end() {
    const auto endIndex = traits_type::end_index(count_);
    return {em_, endIndex, endIndex};
  }

  ConstIterator begin() const {
    return {em_, traits_type::first_index(), traits_type::end_index(count_)};
  }

  ConstIterator end() const {
    const auto endIndex = traits_type::end_index(count_);
    return {em_, endIndex, endIndex};
  }

  ConstIterator cbegin() const {
    return begin();
  }

  ConstIterator cend() const {
    return end();
  }

private:
  EntityManager* em_;
  index_type count_;
};

export template <typename Entity, typename Derived>
class CompositeEntityList : public ReadonlyFactory<Derived> {
public:
  using traits_type = EntityListTraits<Entity>;
  using primary_key_type = typename traits_type::primary_key_type;
  using index_type = typename traits_type::index_type;

protected:
  CompositeEntityList(EntityManager& em, primary_key_type primary_key,
                      index_type count)
      : em_(&em), primary_key_(primary_key), count_(count) {}

public:
  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = EntityHandle<Entity>;
    using difference_type = std::ptrdiff_t;

    Iterator(EntityManager* em, primary_key_type primary_key,
             index_type current, index_type end)
        : em_(em), primary_key_(primary_key), current_(current), end_(end) {
      advance_to_valid();
    }

    value_type operator*() {
      return traits_type::get(*em_, primary_key_, current_);
    }

    Iterator& operator++() {
      current_ = traits_type::next(current_);
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
      while (current_ != end_ && !traits_type::is_valid(traits_type::peek(
                                     *em_, primary_key_, current_))) {
        current_ = traits_type::next(current_);
      }
    }

    EntityManager* em_;
    primary_key_type primary_key_;
    index_type current_;
    index_type end_;
  };

  class ConstIterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const Entity*;
    using difference_type = std::ptrdiff_t;

    ConstIterator(EntityManager* em, primary_key_type primary_key,
                  index_type current, index_type end)
        : em_(em), primary_key_(primary_key), current_(current), end_(end) {
      advance_to_valid();
    }

    const Entity* operator*() const {
      return traits_type::peek(*em_, primary_key_, current_);
    }

    ConstIterator& operator++() {
      current_ = traits_type::next(current_);
      advance_to_valid();
      return *this;
    }

    bool operator!=(const ConstIterator& other) const {
      return current_ != other.current_;
    }

    bool operator==(const ConstIterator& other) const {
      return current_ == other.current_;
    }

  private:
    void advance_to_valid() {
      while (current_ != end_ && !traits_type::is_valid(traits_type::peek(
                                     *em_, primary_key_, current_))) {
        current_ = traits_type::next(current_);
      }
    }

    EntityManager* em_;
    primary_key_type primary_key_;
    index_type current_;
    index_type end_;
  };

  Iterator begin() {
    return {em_, primary_key_, traits_type::first_index(),
            traits_type::end_index(count_)};
  }

  Iterator end() {
    const auto endIndex = traits_type::end_index(count_);
    return {em_, primary_key_, endIndex, endIndex};
  }

  ConstIterator begin() const {
    return {em_, primary_key_, traits_type::first_index(),
            traits_type::end_index(count_)};
  }

  ConstIterator end() const {
    const auto endIndex = traits_type::end_index(count_);
    return {em_, primary_key_, endIndex, endIndex};
  }

  ConstIterator cbegin() const {
    return begin();
  }

  ConstIterator cend() const {
    return end();
  }

private:
  EntityManager* em_;
  primary_key_type primary_key_;
  index_type count_;
};

/**
 * Iterator class for races (1-indexed, 1..num_races).
 * Returns EntityHandle<Race> for RAII auto-save behavior.
 *
 * Skips any race slots that don't exist (sparse iteration).
 */
export class RaceList : public SimpleEntityList<Race, RaceList> {
public:
  using Base = SimpleEntityList<Race, RaceList>;
  using Iterator = typename Base::Iterator;
  using ConstIterator = typename Base::ConstIterator;

  explicit RaceList(EntityManager& em) : Base(em) {}
};

/**
 * Iterator class for stars (0-indexed, 0..numstars-1).
 * Returns EntityHandle<Star> for RAII auto-save behavior.
 */
export class StarList : public SimpleEntityList<Star, StarList> {
public:
  using Base = SimpleEntityList<Star, StarList>;
  using Iterator = typename Base::Iterator;
  using ConstIterator = typename Base::ConstIterator;

  explicit StarList(EntityManager& em) : Base(em) {}
};

/**
 * Iterator class for planets of a star (0-indexed, 0..numplanets-1).
 * Returns EntityHandle<Planet> for RAII auto-save behavior.
 */
export class PlanetList : public CompositeEntityList<Planet, PlanetList> {
public:
  using Base = CompositeEntityList<Planet, PlanetList>;
  using Iterator = typename Base::Iterator;
  using ConstIterator = typename Base::ConstIterator;

  PlanetList(EntityManager& em, starnum_t star, planetnum_t numplanets)
      : Base(em, star, numplanets) {}

  PlanetList(EntityManager& em, starnum_t star, const Star& star_data)
      : Base(em, star, star_data.numplanets()) {}
};

/**
 * Iterator class for commodities (1-indexed, 1..num_commods).
 * Returns EntityHandle<Commod> for RAII auto-save behavior.
 * Only returns valid commodities (non-null, has owner, has amount).
 */
export class CommodList : public SimpleEntityList<Commod, CommodList> {
public:
  using Base = SimpleEntityList<Commod, CommodList>;
  using Iterator = typename Base::Iterator;
  using ConstIterator = typename Base::ConstIterator;

  explicit CommodList(EntityManager& em) : Base(em) {}
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
 *   // Read-only iteration
 *   auto ships = ShipList::readonly(g.entity_manager, ship.ships);
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
export class ShipList : public ReadonlyFactory<ShipList> {
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
