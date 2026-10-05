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
      [&](universe_struct& u) { u.deduct_AP(player_, amount); });
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

bool GameObj::check_commandable(const Ship& ship) {
  const auto result = validate_commandable(ship, player_, governor_, god_);
  if (!result) {
    switch (result.error()) {
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
    return false;
  }

  return true;
}
