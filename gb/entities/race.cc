// SPDX-License-Identifier: Apache-2.0

/// \file race.cc
/// \brief Race and power bloc entity member functions.

module gblib;

bool Race::is_allied_with(player_t p) const noexcept {
  return allied.test(p);
}

void Race::declare_alliance_with(player_t p) noexcept {
  allied.set(p);
}

void Race::rescind_alliance_with(player_t p) noexcept {
  allied.reset(p);
}

bool Race::is_at_war_with(player_t p) const noexcept {
  return atwar.test(p);
}

void Race::declare_war_on(player_t p) noexcept {
  atwar.set(p);
}

void Race::make_peace_with(player_t p) noexcept {
  atwar.reset(p);
}

bool block::is_member(player_t p) const noexcept {
  return is_pledged(p) && is_invited(p);
}

bool block::is_invited(player_t p) const noexcept {
  return invited.test(p);
}

void block::invite(player_t p) noexcept {
  invited.set(p);
}

void block::uninvite(player_t p) noexcept {
  invited.reset(p);
}

bool block::is_pledged(player_t p) const noexcept {
  return pledged.test(p);
}

void block::pledge(player_t p) noexcept {
  pledged.set(p);
}

void block::unpledge(player_t p) noexcept {
  pledged.reset(p);
}

bool block::is_allied_with(player_t p) const noexcept {
  return allied.test(p);
}

void block::declare_alliance_with(player_t p) noexcept {
  allied.set(p);
}

void block::rescind_alliance_with(player_t p) noexcept {
  allied.reset(p);
}

bool block::is_at_war_with(player_t p) const noexcept {
  return atwar.test(p);
}

void block::declare_war_on(player_t p) noexcept {
  atwar.set(p);
}

void block::make_peace_with(player_t p) noexcept {
  atwar.reset(p);
}
