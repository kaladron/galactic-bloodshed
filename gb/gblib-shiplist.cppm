// SPDX-License-Identifier: Apache-2.0

export module gblib:shiplist;

import std;

import :gameobj;
import :ships;
import :services;
import :types;

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
 *   const ShipList ships(g.entity_manager, ship.ships);
 *   for (const auto* ship : ships) {
 *     g.out << ship->name << "\n";
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
    Nested,  ///< Follow ship.ships linked list
    Scope    ///< All ships at current scope (universe/star/planet/ship)
  };

  // Mutable version - returns ShipHandle
  ShipList(EntityManager& em, shipnum_t start,
           IterationType type = IterationType::Nested);
  ShipList(EntityManager& em, const GameObj& g,
           IterationType type = IterationType::Scope);

  // Forward declaration for iterators
  class MutableIterator;
  class ConstIterator;

  // Iterator support
  MutableIterator begin();
  MutableIterator end();

private:
  EntityManager* em{nullptr};
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
