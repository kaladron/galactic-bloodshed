// SPDX-License-Identifier: Apache-2.0

/// \file gameobj.cc
/// \brief GameObj session state and action point deduction implementations.

module;

import std;

module gb.services;

bool GameObj::deduct_ap(starnum_t snum, ap_t amount) {
  if (amount == 0 || god_) return true;
  const auto* star = entity_manager.peek_star(snum);
  if (star->AP(player_) < amount) {
    return false;
  }
  entity_manager.mutate_star(snum, [&](Star& s) { s.AP(player_) -= amount; });
  return true;
}

bool GameObj::deduct_univ_ap(ap_t amount) {
  if (amount == 0 || god_) return true;
  const auto* univ = entity_manager.peek_universe();
  if (univ->get_AP(player_) < amount) {
    return false;
  }
  entity_manager.mutate_universe(
      [&](Universe& u) { u.deduct_AP(player_, amount); });
  return true;
}

std::expected<void, CommandableError> validate_commandable(const Ship& ship,
                                                           player_t player,
                                                           governor_t governor,
                                                           bool god) noexcept {
  if (!god) {
    if (ship.owner() != player) {
      return std::unexpected(CommandableError::NotOwner);
    }
    if (!ship.is_authorized_for(governor)) {
      return std::unexpected(CommandableError::NotAuthorizedGovernor);
    }
  }
  if (!ship.alive()) {
    return std::unexpected(CommandableError::ShipDead);
  }
  if (!ship.active()) {
    return std::unexpected(CommandableError::ShipIrradiated);
  }
  return {};
}

void GameObj::report_commandable_error(const Ship& ship,
                                       CommandableError error) {
  switch (error) {
    case CommandableError::NotOwner:
    case CommandableError::NotAuthorizedGovernor:
      out << std::format("You don't own ship #{}.\n", ship.number());
      break;
    case CommandableError::ShipDead:
      out << std::format("{} has been destroyed.\n", ship);
      break;
    case CommandableError::ShipIrradiated:
      out << std::format("{} is irradiated {}% and inactive.\n", ship,
                         ship.rad());
      break;
  }
}

bool GameObj::check_commandable(const Ship& ship) {
  const auto result = validate_commandable(ship, player_, governor_, god_);
  if (!result) {
    report_commandable_error(ship, result.error());
    return false;
  }

  return true;
}

bool GameObj::reconcile_scope() {
  if (level_ != ScopeLevel::LEVEL_SHIP) {
    carrier_ancestors_.clear();
    return true;
  }
  if (shipno_ > 0) {
    try {
      const auto* ship = entity_manager.peek_ship(shipno_);
      carrier_ancestors_ = entity_manager.ancestor_carriers(shipno_);
      const Ship* root =
          carrier_ancestors_.empty()
              ? ship
              : entity_manager.peek_ship(carrier_ancestors_.back());
      switch (root->whatorbits()) {
        case ScopeLevel::LEVEL_PLAN:
          snum_ = root->storbits();
          pnum_ = root->pnumorbits();
          break;
        case ScopeLevel::LEVEL_STAR:
          snum_ = root->storbits();
          pnum_ = 0;
          break;
        case ScopeLevel::LEVEL_UNIV:
        case ScopeLevel::LEVEL_SHIP:
          snum_ = 0;
          pnum_ = 0;
          break;
      }
      return true;
    } catch (const EntityNotFoundError&) {
    }
  }

  const auto saved_ancestors = std::move(carrier_ancestors_);
  carrier_ancestors_.clear();
  for (const shipnum_t anc_id : saved_ancestors) {
    try {
      (void)entity_manager.peek_ship(anc_id);
      shipno_ = anc_id;
      (void)reconcile_scope();
      return false;
    } catch (const EntityNotFoundError&) {
    }
  }

  shipno_ = 0;
  if (snum_ > 0) {
    try {
      const auto* star = entity_manager.peek_star(snum_);
      if (pnum_ > 0 && pnum_ <= star->numplanets()) {
        try {
          (void)entity_manager.peek_planet(snum_, pnum_);
          level_ = ScopeLevel::LEVEL_PLAN;
          return false;
        } catch (const EntityNotFoundError&) {
        }
      }
      level_ = ScopeLevel::LEVEL_STAR;
      pnum_ = 0;
      return false;
    } catch (const EntityNotFoundError&) {
    }
  }
  level_ = ScopeLevel::LEVEL_UNIV;
  snum_ = 0;
  pnum_ = 0;
  return false;
}
