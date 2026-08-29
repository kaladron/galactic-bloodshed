// SPDX-License-Identifier: Apache-2.0

/// \file race.cc
/// \brief Race and power bloc entity member functions.

module gblib;

bool Race::is_allied_with(player_t p) const noexcept {
  return isset(allied, p);
}

void Race::declare_alliance_with(player_t p) noexcept {
  setbit(allied, p);
}

void Race::rescind_alliance_with(player_t p) noexcept {
  clrbit(allied, p);
}

bool Race::is_at_war_with(player_t p) const noexcept {
  return isset(atwar, p);
}

void Race::declare_war_on(player_t p) noexcept {
  setbit(atwar, p);
}

void Race::make_peace_with(player_t p) noexcept {
  clrbit(atwar, p);
}

bool block::is_invited(player_t p) const noexcept {
  return isset(invite, p);
}

void block::invite_player(player_t p) noexcept {
  setbit(invite, p);
}

void block::cancel_invite(player_t p) noexcept {
  clrbit(invite, p);
}

bool block::is_pledged(player_t p) const noexcept {
  return isset(pledge, p);
}

void block::pledge_player(player_t p) noexcept {
  setbit(pledge, p);
}

void block::unpledge_player(player_t p) noexcept {
  clrbit(pledge, p);
}

bool block::is_allied_with(player_t p) const noexcept {
  return isset(allied, p);
}

void block::declare_alliance_with(player_t p) noexcept {
  setbit(allied, p);
}

void block::rescind_alliance_with(player_t p) noexcept {
  clrbit(allied, p);
}

bool block::is_at_war_with(player_t p) const noexcept {
  return isset(atwar, p);
}

void block::declare_war_on(player_t p) noexcept {
  setbit(atwar, p);
}

void block::make_peace_with(player_t p) noexcept {
  clrbit(atwar, p);
}
