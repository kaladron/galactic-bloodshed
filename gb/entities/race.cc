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

bool block::is_member(player_t p) const noexcept {
  return is_pledged(p) && is_invited(p);
}

bool block::is_invited(player_t p) const noexcept {
  return isset(invited, p);
}

void block::invite(player_t p) noexcept {
  setbit(invited, p);
}

void block::uninvite(player_t p) noexcept {
  clrbit(invited, p);
}

bool block::is_pledged(player_t p) const noexcept {
  return isset(pledged, p);
}

void block::pledge(player_t p) noexcept {
  setbit(pledged, p);
}

void block::unpledge(player_t p) noexcept {
  clrbit(pledged, p);
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
