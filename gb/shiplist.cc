// SPDX-License-Identifier: Apache-2.0

module;

import std;

module gblib;

import :types;
import :services;
import :gameobj;
import :ships;

// ShipHandle implementation using Pimpl to hide EntityHandle<Ship>

struct ShipHandle::Impl {
  EntityHandle<Ship> handle;

  explicit Impl(EntityHandle<Ship>&& h) : handle(std::move(h)) {}
};

ShipHandle::ShipHandle(EntityHandle<Ship>&& handle)
    : pimpl(std::make_unique<Impl>(std::move(handle))) {}

ShipHandle::~ShipHandle() = default;

ShipHandle::ShipHandle(ShipHandle&&) noexcept = default;
ShipHandle& ShipHandle::operator=(ShipHandle&&) noexcept = default;

Ship& ShipHandle::operator*() {
  return *pimpl->handle;
}

const Ship& ShipHandle::operator*() const {
  return *pimpl->handle;
}

Ship* ShipHandle::operator->() {
  return pimpl->handle.operator->();
}

const Ship* ShipHandle::operator->() const {
  return pimpl->handle.operator->();
}

Ship* ShipHandle::get() {
  return pimpl->handle.get();
}

const Ship* ShipHandle::get() const {
  return pimpl->handle.get();
}

const Ship& ShipHandle::peek() const {
  return pimpl->handle.read();
}

void ShipHandle::save() {
  pimpl->handle.save();
}

// ShipList constructors

ShipList::ShipList(EntityManager& em, shipnum_t start, IterationType type)
    : em(&em), start_ship(start), iteration_type(type),
      scope_level(ScopeLevel::LEVEL_UNIV) {}

ShipList::ShipList(EntityManager& em, const GameObj& g, IterationType type)
    : em(&em), start_ship(1),  // Start from ship #1 for scope-based iteration
      iteration_type(type), scope_level(g.level), snum(g.snum), pnum(g.pnum),
      player(g.player) {}

// ShipList iterator methods

ShipList::MutableIterator ShipList::begin() {
  return MutableIterator(*em, start_ship, iteration_type, scope_level, snum,
                         pnum, player);
}

ShipList::MutableIterator ShipList::end() {
  return MutableIterator(*em, 0, iteration_type, scope_level, snum, pnum,
                         player);
}

// Helper to check if a ship matches the current scope
bool ShipList::matches_scope(const Ship& ship) const {
  if (iteration_type == IterationType::Nested) {
    return true;  // Nested iteration doesn't filter by scope
  }

  // Scope-based filtering
  if (!ship.alive) return false;

  switch (scope_level) {
    case ScopeLevel::LEVEL_UNIV:
      return true;  // All ships match universe scope
    case ScopeLevel::LEVEL_STAR:
      return ship.storbits == snum;
    case ScopeLevel::LEVEL_PLAN:
      return ship.storbits == snum && ship.pnumorbits == pnum;
    case ScopeLevel::LEVEL_SHIP:
      // At ship scope, match ships owned by the player
      return ship.owner == player;
  }
  return false;
}

// MutableIterator implementation

ShipList::MutableIterator::MutableIterator(EntityManager& em, shipnum_t current,
                                           IterationType type, ScopeLevel level,
                                           starnum_t snum, planetnum_t pnum,
                                           player_t player)
    : em(em), current(current), type(type), scope_level(level), snum(snum),
      pnum(pnum), player(player) {
  // Advance to first matching ship
  if (current != 0) {
    advance_to_next_match();
  }
}

ShipList::MutableIterator& ShipList::MutableIterator::operator++() {
  if (current == 0) return *this;

  if (type == IterationType::Nested) {
    // Follow nextship linked list
    const auto* ship = em.peek_ship(current);
    current = ship ? ship->nextship : 0;
  } else {
    // Scope-based: increment to next ship number
    ++current;
    if (current > em.num_ships()) {
      current = 0;
    }
  }

  advance_to_next_match();
  return *this;
}

ShipHandle ShipList::MutableIterator::operator*() {
  return ShipHandle(em.get_ship(current));
}

bool ShipList::MutableIterator::operator==(const MutableIterator& other) const {
  return current == other.current;
}

bool ShipList::MutableIterator::operator!=(const MutableIterator& other) const {
  return !(*this == other);
}

void ShipList::MutableIterator::advance_to_next_match() {
  while (current != 0) {
    const auto* ship = em.peek_ship(current);
    if (!ship) {
      current = 0;
      return;
    }

    if (matches_scope(*ship)) {
      return;  // Found a matching ship
    }

    // Move to next ship
    if (type == IterationType::Nested) {
      current = ship->nextship;
    } else {
      ++current;
      if (current > em.num_ships()) {
        current = 0;
      }
    }
  }
}

bool ShipList::MutableIterator::matches_scope(const Ship& ship) const {
  if (type == IterationType::Nested) {
    return true;  // Nested iteration doesn't filter by scope
  }

  // Scope-based filtering
  if (!ship.alive) return false;

  switch (scope_level) {
    case ScopeLevel::LEVEL_UNIV:
      return true;  // All ships match universe scope
    case ScopeLevel::LEVEL_STAR:
      return ship.storbits == snum;
    case ScopeLevel::LEVEL_PLAN:
      return ship.storbits == snum && ship.pnumorbits == pnum;
    case ScopeLevel::LEVEL_SHIP:
      // At ship scope, match ships owned by the player
      return ship.owner == player;
  }
  return false;
}
