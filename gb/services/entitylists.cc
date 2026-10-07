// SPDX-License-Identifier: Apache-2.0

/// \file entitylists.cc
/// \brief Implementations for entity list iterators.

module;

import std;

module gb.services;

// ShipList constructors

ShipList::ShipList(EntityManager& em, const ScopeContext& ctx,
                   IterationType type)
    : em_(&em), alive_only_(type != IterationType::All) {
  if (type == IterationType::All) {
    ship_ids_ = em.ships_all();
  } else if (type == IterationType::AllAlive) {
    ship_ids_ = em.ships_alive();
  } else {
    switch (ctx.level) {
      case ScopeLevel::LEVEL_UNIV:
        ship_ids_ = em.ships_alive();
        break;
      case ScopeLevel::LEVEL_STAR:
        ship_ids_ = em.ships_in_star_system(ctx.snum);
        break;
      case ScopeLevel::LEVEL_PLAN:
        ship_ids_ = em.ships_on_planet(ctx.snum, ctx.pnum);
        break;
      case ScopeLevel::LEVEL_SHIP:
        ship_ids_ = em.ships_by_owner(ctx.player);
        break;
    }
  }
}

ShipList::ShipList(EntityManager& em, const GameObj& g, IterationType type)
    : ShipList(em, g.scope_context(), type) {}

ShipList::ShipList(const GameObj& g, IterationType type)
    : ShipList(g.entity_manager, g.scope_context(), type) {}

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
      const Ship& ship = *em_->peek_ship(*it_);
      if (!alive_only_ || ship.alive()) {
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
      const Ship& ship = *em_->peek_ship(*it_);
      if (!alive_only_ || ship.alive()) {
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

ShipHandle ShipList::acquire_handle(EntityManager& em, shipnum_t num) {
  return ShipHandle(em.get_ship(num));
}

std::expected<ShipHandle, CommandableError>
resolve_explicit_ship(EntityManager& em, const ScopeContext& ctx,
                      std::string_view filter, bool require_active) {
  if (!GB::is_ship_number_filter(filter)) {
    return std::unexpected(CommandableError::NotOwner);
  }
  const auto shipno = GB::parse_ship_selection(filter);
  if (!shipno || *shipno <= 0) {
    return std::unexpected(CommandableError::NotOwner);
  }
  const Ship* ship = nullptr;
  try {
    ship = em.peek_ship(*shipno);
  } catch (const EntityNotFoundError&) {
    return std::unexpected(CommandableError::NotOwner);
  }
  const auto check =
      validate_commandable(*ship, ctx.player, ctx.governor, ctx.god);
  if (!check &&
      (require_active || check.error() != CommandableError::ShipIrradiated)) {
    return std::unexpected(check.error());
  }
  return ShipList::acquire_handle(em, *shipno);
}

ScopedCommandableShips::ScopedCommandableShips(EntityManager& em,
                                               const ScopeContext& ctx,
                                               std::string_view filter,
                                               bool require_active)
    : em_(&em), ctx_(ctx), filter_(filter), require_active_(require_active) {
  switch (ctx.level) {
    case ScopeLevel::LEVEL_UNIV:
      ship_ids_ = em.ships_alive();
      break;
    case ScopeLevel::LEVEL_STAR:
      ship_ids_ = em.ships_in_star_system(ctx.snum);
      break;
    case ScopeLevel::LEVEL_PLAN:
      ship_ids_ = em.ships_on_planet(ctx.snum, ctx.pnum);
      break;
    case ScopeLevel::LEVEL_SHIP:
      ship_ids_ = em.ships_by_owner(ctx.player);
      break;
  }
}

ScopedCommandableShips::ScopedCommandableShips(GameObj& g,
                                               std::string_view filter,
                                               bool require_active)
    : ScopedCommandableShips(g.entity_manager, g.scope_context(), filter,
                             require_active) {
  if (GB::is_ship_number_filter(filter) && begin() == end()) {
    if (const auto shipno = GB::parse_ship_selection(filter);
        shipno && *shipno > 0) {
      try {
        const Ship& ship = *g.entity_manager.peek_ship(*shipno);
        const auto check =
            validate_commandable(ship, g.player(), g.governor(), g.god());
        if (!check && (require_active ||
                       check.error() != CommandableError::ShipIrradiated)) {
          (void)g.check_commandable(ship);
        }
      } catch (const EntityNotFoundError&) {
        g.out << std::format("You don't own ship #{}.\n", *shipno);
      }
    }
  }
}

ScopedCommandableShips::Iterator::Iterator(
    EntityManager& em, const ScopeContext& ctx, std::string_view filter,
    bool require_active, std::vector<shipnum_t>::const_iterator it,
    std::vector<shipnum_t>::const_iterator end)
    : em_(&em), ctx_(ctx), filter_(filter), require_active_(require_active),
      it_(it), end_(end) {
  advance_to_valid();
}

void ScopedCommandableShips::Iterator::advance_to_valid() {
  while (it_ != end_) {
    try {
      const Ship& ship = *em_->peek_ship(*it_);
      const auto check =
          validate_commandable(ship, ctx_.player, ctx_.governor, ctx_.god);
      const bool commandable =
          check.has_value() ||
          (!require_active_ &&
           check.error() == CommandableError::ShipIrradiated);
      if (commandable && GB::ship_matches_filter(filter_, ship)) {
        return;
      }
    } catch (const EntityNotFoundError&) {
      // Ship was hard-deleted mid-iteration; skip it.
    }
    ++it_;
  }
}

ScopedCommandableShips::Iterator&
ScopedCommandableShips::Iterator::operator++() {
  if (GB::is_ship_number_filter(filter_)) {
    it_ = end_;
    return *this;
  }
  ++it_;
  advance_to_valid();
  return *this;
}

ScopedCommandableShips::Iterator
ScopedCommandableShips::Iterator::operator++(int) {
  Iterator tmp = *this;
  ++(*this);
  return tmp;
}

ShipHandle ScopedCommandableShips::Iterator::operator*() const {
  return ShipList::acquire_handle(*em_, *it_);
}

bool ScopedCommandableShips::Iterator::operator==(const Iterator& other) const {
  return it_ == other.it_;
}

bool ScopedCommandableShips::Iterator::operator!=(const Iterator& other) const {
  return it_ != other.it_;
}

ScopedCommandableShips::Iterator ScopedCommandableShips::begin() const {
  return Iterator(*em_, ctx_, filter_, require_active_, ship_ids_.begin(),
                  ship_ids_.end());
}

ScopedCommandableShips::Iterator ScopedCommandableShips::end() const {
  return Iterator(*em_, ctx_, filter_, require_active_, ship_ids_.end(),
                  ship_ids_.end());
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
