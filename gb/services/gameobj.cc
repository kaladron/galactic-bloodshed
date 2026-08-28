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
  if (player_ == 0) return false;
  try {
    const auto* univ = entity_manager.peek_universe();
    if (!univ || univ->AP[player_.value - 1] < amount) {
      return false;
    }
    entity_manager.mutate_universe(
        [&](universe_struct& u) { u.AP[player_.value - 1] -= amount; });
    return true;
  } catch (const EntityNotFoundError&) {
    return false;
  }
}
