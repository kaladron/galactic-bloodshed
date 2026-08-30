// SPDX-License-Identifier: Apache-2.0

/// \file star.cc
/// \brief Star class member functions.

module gblib;

bool Star::control(player_t Playernum, governor_t Governor) const {
  return Governor == 0 || star_struct.governor[Playernum] == Governor;
}

bool Star::is_explored_by(player_t p) const noexcept {
  return isset(star_struct.explored, p);
}

void Star::mark_explored_by(player_t p) noexcept {
  setbit(star_struct.explored, p);
}

bool Star::is_explored() const noexcept {
  return star_struct.explored != 0;
}

bool Star::is_inhabited_by(player_t p) const noexcept {
  return isset(star_struct.inhabited, p);
}

void Star::mark_inhabited_by(player_t p) noexcept {
  setbit(star_struct.inhabited, p);
}

void Star::clear_inhabited_by(player_t p) noexcept {
  clrbit(star_struct.inhabited, p);
}

bool Star::is_inhabited() const noexcept {
  return star_struct.inhabited != 0;
}

void Star::clear_all_inhabitants() noexcept {
  star_struct.inhabited = 0;
}

std::optional<planetnum_t> Star::get_random_planet_index() const {
  if (star_struct.pnames.empty()) {
    return std::nullopt;
  }
  return planetnum_t{
      static_cast<unsigned int>(int_rand(0, star_struct.pnames.size() - 1))};
}
