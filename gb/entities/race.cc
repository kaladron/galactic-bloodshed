// SPDX-License-Identifier: Apache-2.0

/// \file race.cc
/// \brief Race and power bloc entity member functions.

module gb.entities;

bool Race::is_allied_with(player_t p) const noexcept {
  return allied.contains(p);
}

void Race::declare_alliance_with(player_t p) noexcept {
  allied.insert(p);
}

void Race::rescind_alliance_with(player_t p) noexcept {
  allied.erase(p);
}

bool Race::is_at_war_with(player_t p) const noexcept {
  return atwar.contains(p);
}

void Race::declare_war_on(player_t p) noexcept {
  atwar.insert(p);
}

void Race::make_peace_with(player_t p) noexcept {
  atwar.erase(p);
}

bool Block::is_member(player_t p) const noexcept {
  return is_pledged(p) && is_invited(p);
}

bool Block::is_invited(player_t p) const noexcept {
  return data_.invited.contains(p);
}

void Block::invite(player_t p) noexcept {
  data_.invited.insert(p);
}

void Block::uninvite(player_t p) noexcept {
  data_.invited.erase(p);
}

bool Block::is_pledged(player_t p) const noexcept {
  return data_.pledged.contains(p);
}

void Block::pledge(player_t p) noexcept {
  data_.pledged.insert(p);
}

void Block::unpledge(player_t p) noexcept {
  data_.pledged.erase(p);
}
