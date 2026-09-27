// SPDX-License-Identifier: Apache-2.0

/// \file entitylists.cc
/// \brief Implementations for entity list iterators.

module;

import std;

module gb.services;

// ShipList constructors

ShipList::ShipList(EntityManager& em, const GameObj& g, IterationType type)
    : em_(&em), alive_only_(type != IterationType::All) {
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
        ship_ids_ = em.ships_in_star_system(g.snum());
        break;
      case ScopeLevel::LEVEL_PLAN:
        ship_ids_ = em.ships_on_planet(g.snum(), g.pnum());
        break;
      case ScopeLevel::LEVEL_SHIP:
        ship_ids_ = em.ships_by_owner(g.player());
        break;
    }
  }
}

ShipList::ShipList(const GameObj& g, IterationType type)
    : ShipList(g.entity_manager, g, type) {}

ShipList::ShipList(EntityManager& em, ScopeLevel scope)
    : em_(&em), ship_ids_(em.ships_at_scope(scope)) {}

ShipList::ShipList(EntityManager& em, starnum_t star_id)
    : em_(&em), ship_ids_(em.ships_in_star(star_id)) {}

ShipList::ShipList(EntityManager& em, starnum_t star_id, planetnum_t planet_id)
    : em_(&em), ship_ids_(em.ships_on_planet(star_id, planet_id)) {}

ShipList::ShipList(EntityManager& em, IterationType type)
    : em_(&em), alive_only_(type != IterationType::All) {
  if (type == IterationType::All) {
    ship_ids_ = em.ships_all();
  } else {
    ship_ids_ = em.ships_alive();
  }
}

ShipList::ShipList(EntityManager& em, std::vector<shipnum_t> ship_ids,
                   bool alive_only)
    : em_(&em), ship_ids_(std::move(ship_ids)), alive_only_(alive_only) {}

ShipList ShipList::in_carrier(EntityManager& em, shipnum_t carrier_id) {
  return ShipList(em, em.ships_in_hangar(carrier_id));
}

const ShipList ShipList::readonly_in_carrier(EntityManager& em,
                                             shipnum_t carrier_id) {
  return ShipList(em, em.ships_in_hangar(carrier_id));
}

// ShipList iterator methods

ShipList::MutableIterator ShipList::begin() {
  return MutableIterator(*em_, ship_ids_.begin(), ship_ids_.end(), alive_only_);
}

ShipList::MutableIterator ShipList::end() {
  return MutableIterator(*em_, ship_ids_.end(), ship_ids_.end(), alive_only_);
}

ShipList::ConstIterator ShipList::begin() const {
  return ConstIterator(*em_, ship_ids_.begin(), ship_ids_.end(), alive_only_);
}

ShipList::ConstIterator ShipList::end() const {
  return ConstIterator(*em_, ship_ids_.end(), ship_ids_.end(), alive_only_);
}

ShipList::ConstIterator ShipList::cbegin() const {
  return begin();
}

ShipList::ConstIterator ShipList::cend() const {
  return end();
}

// MutableIterator implementation

ShipList::MutableIterator::MutableIterator(
    EntityManager& em, std::vector<shipnum_t>::const_iterator it,
    std::vector<shipnum_t>::const_iterator end, bool alive_only)
    : em_(&em), it_(it), end_(end), alive_only_(alive_only) {
  advance_to_valid();
}

void ShipList::MutableIterator::advance_to_valid() {
  while (it_ != end_) {
    try {
      const Ship* ship = em_->peek_ship(*it_);
      if (ship && (!alive_only_ || ship->alive())) {
        return;
      }
    } catch (const EntityNotFoundError&) {
      // Ship was hard-deleted mid-iteration; skip it
    }
    ++it_;
  }
}

ShipList::MutableIterator& ShipList::MutableIterator::operator++() {
  ++it_;
  advance_to_valid();
  return *this;
}

ShipList::MutableIterator ShipList::MutableIterator::operator++(int) {
  MutableIterator tmp = *this;
  ++(*this);
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
    EntityManager& em, std::vector<shipnum_t>::const_iterator it,
    std::vector<shipnum_t>::const_iterator end, bool alive_only)
    : em_(&em), it_(it), end_(end), alive_only_(alive_only) {
  advance_to_valid();
}

void ShipList::ConstIterator::advance_to_valid() {
  while (it_ != end_) {
    try {
      const Ship* ship = em_->peek_ship(*it_);
      if (ship && (!alive_only_ || ship->alive())) {
        return;
      }
    } catch (const EntityNotFoundError&) {
      // Ship was hard-deleted mid-iteration; skip it
    }
    ++it_;
  }
}

ShipList::ConstIterator& ShipList::ConstIterator::operator++() {
  ++it_;
  advance_to_valid();
  return *this;
}

ShipList::ConstIterator ShipList::ConstIterator::operator++(int) {
  ConstIterator tmp = *this;
  ++(*this);
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

std::tuple<player_t, governor_t> getracenum(EntityManager& entity_manager,
                                            const std::string& racepass,
                                            const std::string& govpass) {
  for (auto race_handle : RaceList(entity_manager)) {
    const auto& race = race_handle.read();
    if (racepass == race.password) {
      for (auto [j, gov] : race.active_governors()) {
        if (!gov.password.empty() && govpass == gov.password) {
          return {race.Playernum, j};
        }
      }
    }
  }
  return {0, 0};
}

player_t get_player(EntityManager& em, const std::string& name) {
  return em.find_player_by_name(name).value_or(player_t{0});
}
