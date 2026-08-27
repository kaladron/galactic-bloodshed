// SPDX-License-Identifier: Apache-2.0

module;

import std;

module gblib;

import :types;
import :services;
import :gameobj;
import :ships;
#undef stdout

// ShipList constructors

ShipList::ShipList(EntityManager& em, shipnum_t start, IterationType type)
    : em_(&em) {
  if (type == IterationType::Nested) {
    for (shipnum_t curr = start; curr != 0;) {
      const auto* ship = em.peek_ship(curr);
      if (!ship) break;
      ship_ids_.push_back(curr);
      curr = ship->nextship();
    }
  } else if (type == IterationType::Scope) {
    ship_ids_ = em.ships_alive();
  } else if (type == IterationType::AllAlive) {
    ship_ids_ = em.ships_alive();
  } else if (type == IterationType::All) {
    ship_ids_ = em.ships_all();
  }
}

ShipList::ShipList(EntityManager& em, const GameObj& g, IterationType type)
    : em_(&em) {
  if (type == IterationType::All) {
    ship_ids_ = em.ships_all();
  } else if (type == IterationType::AllAlive) {
    ship_ids_ = em.ships_alive();
  } else {
    switch (g.level()) {
      case ScopeLevel::LEVEL_UNIV:
        ship_ids_ = em.ships_alive();
        break;
      case ScopeLevel::LEVEL_STAR:
        ship_ids_ = em.ships_in_star_system(g.snum(), /*alive_only=*/true);
        break;
      case ScopeLevel::LEVEL_PLAN:
        ship_ids_ = em.ships_on_planet(g.snum(), g.pnum(), /*alive_only=*/true);
        break;
      case ScopeLevel::LEVEL_SHIP:
        ship_ids_ = em.ships_by_owner(g.player(), /*alive_only=*/true);
        break;
    }
  }
}

ShipList::ShipList(const GameObj& g, IterationType type)
    : ShipList(g.entity_manager, g, type) {}

ShipList::ShipList(EntityManager& em, ScopeLevel scope, bool alive_only)
    : em_(&em), ship_ids_(em.ships_at_scope(scope, alive_only)) {}

ShipList::ShipList(EntityManager& em, starnum_t star_id, bool alive_only)
    : em_(&em), ship_ids_(em.ships_in_star(star_id, alive_only)) {}

ShipList::ShipList(EntityManager& em, starnum_t star_id, planetnum_t planet_id,
                   bool alive_only)
    : em_(&em), ship_ids_(em.ships_on_planet(star_id, planet_id, alive_only)) {}

ShipList::ShipList(EntityManager& em, IterationType type) : em_(&em) {
  if (type == IterationType::All) {
    ship_ids_ = em.ships_all();
  } else {
    ship_ids_ = em.ships_alive();
  }
}

ShipList::ShipList(EntityManager& em, std::vector<shipnum_t> ship_ids)
    : em_(&em), ship_ids_(std::move(ship_ids)) {}

// ShipList iterator methods

ShipList::MutableIterator ShipList::begin() {
  return MutableIterator(*em_, ship_ids_.begin());
}

ShipList::MutableIterator ShipList::end() {
  return MutableIterator(*em_, ship_ids_.end());
}

ShipList::ConstIterator ShipList::begin() const {
  return ConstIterator(*em_, ship_ids_.begin());
}

ShipList::ConstIterator ShipList::end() const {
  return ConstIterator(*em_, ship_ids_.end());
}

ShipList::ConstIterator ShipList::cbegin() const {
  return begin();
}

ShipList::ConstIterator ShipList::cend() const {
  return end();
}

// MutableIterator implementation

ShipList::MutableIterator::MutableIterator(
    EntityManager& em, std::vector<shipnum_t>::const_iterator it)
    : em_(&em), it_(it) {}

ShipList::MutableIterator& ShipList::MutableIterator::operator++() {
  ++it_;
  return *this;
}

ShipList::MutableIterator ShipList::MutableIterator::operator++(int) {
  MutableIterator tmp = *this;
  ++it_;
  return tmp;
}

ShipHandle ShipList::MutableIterator::operator*() const {
  return ShipHandle(em_->get_ship(*it_));
}

bool ShipList::MutableIterator::operator==(const MutableIterator& other) const {
  return it_ == other.it_;
}

bool ShipList::MutableIterator::operator!=(const MutableIterator& other) const {
  return it_ != other.it_;
}

// ConstIterator implementation

ShipList::ConstIterator::ConstIterator(
    EntityManager& em, std::vector<shipnum_t>::const_iterator it)
    : em_(&em), it_(it) {}

ShipList::ConstIterator& ShipList::ConstIterator::operator++() {
  ++it_;
  return *this;
}

ShipList::ConstIterator ShipList::ConstIterator::operator++(int) {
  ConstIterator tmp = *this;
  ++it_;
  return tmp;
}

const Ship& ShipList::ConstIterator::operator*() const {
  return *em_->peek_ship(*it_);
}

const Ship* ShipList::ConstIterator::operator->() const {
  return em_->peek_ship(*it_);
}

bool ShipList::ConstIterator::operator==(const ConstIterator& other) const {
  return it_ == other.it_;
}

bool ShipList::ConstIterator::operator!=(const ConstIterator& other) const {
  return it_ != other.it_;
}
