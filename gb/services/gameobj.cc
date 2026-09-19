// SPDX-License-Identifier: Apache-2.0

/// \file gameobj.cc
/// \brief GameObj session state and action point deduction implementations.

module;

import std;

module gblib;

bool GameObj::deduct_ap(starnum_t snum, ap_t amount) {
  if (amount == 0 || god_) return true;
  try {
    const auto* star = entity_manager.peek_star(snum);
    if (!star || star->AP(player_) < amount) {
      return false;
    }
    entity_manager.mutate_star(snum, [&](Star& s) { s.AP(player_) -= amount; });
    return true;
  } catch (const EntityNotFoundError&) {
    return false;
  }
}

bool GameObj::deduct_univ_ap(ap_t amount) {
  if (amount == 0 || god_) return true;
  if (player_ == 0 || player_ > MAXPLAYERS) return false;
  try {
    const auto* univ = entity_manager.peek_universe();
    if (!univ || univ->AP[player_] < amount) {
      return false;
    }
    entity_manager.mutate_universe(
        [&](universe_struct& u) { u.AP[player_] -= amount; });
    return true;
  } catch (const EntityNotFoundError&) {
    return false;
  }
}

bool GameObj::check_commandable(const Ship& ship) {
  if (!ship.alive()) {
    out << std::format("{} has been destroyed.\n", ship);
    return false;
  }

  if (ship.owner() != player_ || !ship.is_authorized_for(governor_)) {
    notify_dont_own_ship(*this, ship.number());
    return false;
  }

  if (!ship.active()) {
    out << std::format("{} is irradiated {}% and inactive.\n", ship,
                       ship.rad());
    return false;
  }

  return true;
}
