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

void GameObj::emulate(player_t target_player, governor_t target_governor) {
  emulated_deity_ = EmulatedIdentity{
      .player = player_,
      .governor = governor_,
  };
  player_ = target_player;
  governor_ = target_governor;
  god_ = false;
  race = entity_manager.peek_race(target_player);
  reconcile_scope();
}

SessionStatus GameObj::reconcile_session() {
  if (player_ <= 0) {
    race = nullptr;
    return SessionStatus::Active;
  }
  const Race* current = entity_manager.peek_race(player_);
  if (!current->dissolved && current->has_governor(governor_)) {
    race = current;
    return SessionStatus::Active;
  }
  if (emulated_deity_.has_value()) {
    const auto origin = *std::exchange(emulated_deity_, std::nullopt);
    const Race* deity_race = entity_manager.peek_race(origin.player);
    if (!deity_race->dissolved && deity_race->has_governor(origin.governor)) {
      player_ = origin.player;
      governor_ = origin.governor;
      god_ = deity_race->God;
      race = deity_race;
      return SessionStatus::EmulationRestored;
    }
  }
  race = nullptr;
  disconnect_requested_ = true;
  return SessionStatus::Invalidated;
}

std::string GameObj::format_emulation_restored_message() const {
  return std::format("Emulated session ended; restored {} \"{}\" [{},{}].\n",
                     race->name, current_governor().name, player_, governor_);
}

std::optional<player_t>
GameObj::resolve_active_player(const std::string& token) {
  const player_t target = get_player(entity_manager, token);
  if (target == 0) {
    out << "No such player.\n";
    return std::nullopt;
  }
  if (entity_manager.peek_race(target)->dissolved) {
    out << "That player has dissolved.\n";
    return std::nullopt;
  }
  return target;
}

bool GameObj::is_valid_ship_scope(const Ship& ship) const noexcept {
  if (!ship.alive()) return false;
  if (god_) return true;
  return ship.owner() == player_;
}

void GameObj::sync_celestial_coords_from_ship(const Ship& root) noexcept {
  switch (root.whatorbits()) {
    case ScopeLevel::LEVEL_PLAN:
      snum_ = root.storbits();
      pnum_ = root.pnumorbits();
      break;
    case ScopeLevel::LEVEL_STAR:
      snum_ = root.storbits();
      pnum_ = 0;
      break;
    case ScopeLevel::LEVEL_UNIV:
    case ScopeLevel::LEVEL_SHIP:
      snum_ = 0;
      pnum_ = 0;
      break;
  }
}

void GameObj::step_up_from_ship_scope() {
  const auto saved_ancestors = std::move(carrier_ancestors_);
  carrier_ancestors_.clear();
  for (const shipnum_t anc_id : saved_ancestors) {
    try {
      const auto* anc = entity_manager.peek_ship(anc_id);
      if (is_valid_ship_scope(*anc)) {
        shipno_ = anc_id;
        carrier_ancestors_ = entity_manager.ancestor_carriers(anc_id);
        const Ship* root =
            carrier_ancestors_.empty()
                ? anc
                : entity_manager.peek_ship(carrier_ancestors_.back());
        sync_celestial_coords_from_ship(*root);
        return;
      }
    } catch (const EntityNotFoundError&) {
    }
  }
  shipno_ = 0;
  level_ = ScopeLevel::LEVEL_PLAN;
}

void GameObj::reconcile_celestial_scope() {
  if (level_ == ScopeLevel::LEVEL_SHIP || level_ == ScopeLevel::LEVEL_UNIV) {
    return;
  }
  if (snum_ <= 0) {
    level_ = ScopeLevel::LEVEL_UNIV;
    snum_ = 0;
    pnum_ = 0;
    return;
  }
  const auto* star = entity_manager.peek_star(snum_);
  if (level_ == ScopeLevel::LEVEL_PLAN) {
    if (pnum_ >= 1 && pnum_ <= star->numplanets()) {
      const auto* planet = entity_manager.peek_planet(snum_, pnum_);
      if (god_ || planet->info(player_).explored) {
        return;
      }
    }
    level_ = ScopeLevel::LEVEL_STAR;
    pnum_ = 0;
  }
  if (!god_ && !star->is_explored_by(player_)) {
    level_ = ScopeLevel::LEVEL_UNIV;
    snum_ = 0;
    pnum_ = 0;
  }
}

ShipScopeStatus GameObj::reconcile_ship_scope_internal() {
  if (shipno_ <= 0) {
    step_up_from_ship_scope();
    reconcile_celestial_scope();
    return ShipScopeStatus::NoShipScoped;
  }
  try {
    const auto* ship = entity_manager.peek_ship(shipno_);
    auto ancestors = entity_manager.ancestor_carriers(shipno_);
    const Ship* root =
        ancestors.empty() ? ship : entity_manager.peek_ship(ancestors.back());
    sync_celestial_coords_from_ship(*root);
    carrier_ancestors_ = std::move(ancestors);
    if (!ship->alive()) {
      step_up_from_ship_scope();
      reconcile_celestial_scope();
      return ShipScopeStatus::ShipMissing;
    }
    if (!god_ && ship->owner() != player_) {
      step_up_from_ship_scope();
      reconcile_celestial_scope();
      return ShipScopeStatus::NotOwned;
    }
    return ShipScopeStatus::Valid;
  } catch (const EntityNotFoundError&) {
    step_up_from_ship_scope();
    reconcile_celestial_scope();
    return ShipScopeStatus::ShipMissing;
  }
}

ShipScopeStatus GameObj::reconcile_ship_scope() {
  if (level_ != ScopeLevel::LEVEL_SHIP) {
    return ShipScopeStatus::Valid;
  }
  return reconcile_ship_scope_internal();
}

void GameObj::reconcile_scope() {
  if (level_ == ScopeLevel::LEVEL_SHIP) {
    reconcile_ship_scope_internal();
    return;
  }
  carrier_ancestors_.clear();
  reconcile_celestial_scope();
}
