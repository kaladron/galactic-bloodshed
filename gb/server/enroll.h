// SPDX-License-Identifier: Apache-2.0

/// \file enroll.h
/// \brief Header for race enrollment functions.

#ifndef ENROLL_H
#define ENROLL_H

import gb.entities;
import gb.services;

template <typename Range>
std::optional<std::pair<int, int>>
find_suitable_enrol_planet(EntityManager& entity_manager, int numstars,
                           int num_players, PlanetType ppref,
                           const Range& star_order) {
  for (auto star : star_order) {
    if (star < 0 || star >= numstars) continue;
    const auto* star_ptr = entity_manager.peek_star(star);
    if (!star_ptr) continue;

    /* skip over inhabited stars - or stars with just one planet! */
    if (star_ptr->inhabited() != 0 || star_ptr->numplanets() < 2) continue;

    for (int pnum = 0; pnum < star_ptr->numplanets(); ++pnum) {
      const auto* planet_ptr = entity_manager.peek_planet(star, pnum);
      if (!planet_ptr) continue;

      if (planet_ptr->type() == ppref && star_ptr->numplanets() != 1) {
        bool vacant = true;
        for (int i = 1; i <= num_players; ++i) {
          if (planet_ptr->info(player_t{i}).numsectsowned > 0) {
            vacant = false;
            break;
          }
        }
        if (vacant && planet_ptr->conditions(RTEMP) >= -50 &&
            planet_ptr->conditions(RTEMP) <= 50) {
          return std::make_pair(static_cast<int>(star), pnum);
        }
      }
    }
  }
  return std::nullopt;
}

int enroll(int argc, const char* argv[]);
void process(int argc, const char* argv[]);

#endif  // ENROLL_H
