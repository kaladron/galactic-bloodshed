// SPDX-License-Identifier: Apache-2.0

/// \file star.cc
/// \brief Star class member functions.

module gb.entities;

bool Star::control(player_t Playernum, governor_t Governor) const {
  return Race::is_leader(Governor) || governor(Playernum) == Governor;
}

bool Star::is_explored_by(player_t p) const noexcept {
  return data_.explored.contains(p);
}

void Star::mark_explored_by(player_t p) noexcept {
  data_.explored.insert(p);
}

void Star::clear_explored_by(player_t p) noexcept {
  data_.explored.erase(p);
}

bool Star::is_explored() const noexcept {
  return !data_.explored.empty();
}

void Star::clear_all_explored() noexcept {
  data_.explored.clear();
}

bool Star::is_inhabited_by(player_t p) const noexcept {
  return data_.inhabited.contains(p);
}

void Star::mark_inhabited_by(player_t p) noexcept {
  data_.inhabited.insert(p);
}

void Star::clear_inhabited_by(player_t p) noexcept {
  data_.inhabited.erase(p);
}

bool Star::is_inhabited() const noexcept {
  return !data_.inhabited.empty();
}

void Star::clear_all_inhabitants() noexcept {
  data_.inhabited.clear();
}

planetnum_t Star::get_random_planet_index() const {
  return planetnum_t{
      static_cast<unsigned int>(int_rand(1, data_.pnames.size()))};
}
