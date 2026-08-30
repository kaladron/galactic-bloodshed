// SPDX-License-Identifier: Apache-2.0

/// \file planet.cc
/// \brief Planet domain object implementations.

module;

import std;
#undef stdout

module gblib;

/**
 * @brief Performs a revolt on a planet.
 *
 * This function calculates the number of sectors that revolt on a planet owned
 * by a victim player and assigns them to an agent player. The revolt rate is
 * determined by the tax rate of the victim player. If the revolt is successful,
 * the sectors are transferred to the agent player, some population is killed,
 * and all troops are destroyed. The number of revolted sectors is returned.
 *
 * @param pl The planet on which the revolt is performed.
 * @param victim The player who currently owns the planet.
 * @param agent The player who will receive the revolted sectors.
 * @return The number of sectors that revolted.
 */
int revolt(Planet& pl, EntityManager& entity_manager, const starnum_t snum,
           const planetnum_t pnum, const player_t victim,
           const player_t agent) {
  int revolted_sectors = 0;

  try {
    entity_manager.with_race(victim, [&](const Race& victim_race) {
      entity_manager.mutate_sectormap(snum, pnum, [&](SectorMap& smap) {
        for (auto [c, s] : smap.indexed_sectors()) {
          if (s.get_owner() != victim || s.get_popn() == 0) continue;

          // Revolt rate is a function of tax rate.
          if (!success(pl.info(victim).tax)) continue;

          if (long_rand(1, s.get_popn()) <=
              10L * victim_race.fighters * s.get_troops())
            continue;

          // Revolt successful.
          s.set_owner(agent); /* enemy gets it */
          s.subtract_popn(
              long_rand(0, s.get_popn() - 1)); /* some people killed */
          s.set_troops(0);                     /* all troops destroyed */
          pl.info(victim).numsectsowned -= 1;
          pl.info(agent).numsectsowned += 1;
          pl.info(victim).mob_points -= s.get_mobilization();
          pl.info(agent).mob_points += s.get_mobilization();
          revolted_sectors++;
        }
      });
    });
  } catch (const EntityNotFoundError&) {
    return 0;
  }

  return revolted_sectors;
}

/**
 * @brief Updates the orbital position of a planet and its orbiting ships.
 *
 * This function calculates the new orbital position of a planet based on
 * Kepler's Third Law for circular orbits ($T^2 \propto r^3$).
 *
 * The orbit is counter-clockwise. While subtracting from the angular phase
 * usually results in clockwise rotation in standard Cartesian systems, Galactic
 * Bloodshed uses a left-handed coordinate system where Y increases downwards.
 * In this system, decreasing the angle results in counter-clockwise motion.
 *
 * It also moves all ships currently in orbit around the planet by the same
 * displacement.
 *
 * @param entity_manager The entity manager for accessing ship data.
 * @param star The star that the planet orbits.
 * @param planet The planet object to be moved.
 */
void moveplanet(EntityManager& entity_manager, const Star& star,
                Planet& planet) {
  double dist = std::hypot(planet.ypos(), planet.xpos());

  double phase = std::atan2(planet.ypos(), planet.xpos());
  double period = dist * std::sqrt((dist / (SYSTEMGRAVCONST * star.gravity())));

  double xadd = (dist * std::cos(((-1. / period) + phase))) - planet.xpos();
  double yadd = (dist * std::sin(((-1. / period) + phase))) - planet.ypos();

  /* adjust ships in orbit around the planet */
  for (auto ship_handle : ShipList(entity_manager, planet.ships())) {
    ship_handle->xpos() += xadd;
    ship_handle->ypos() += yadd;
  }

  planet.xpos() += xadd;
  planet.ypos() += yadd;
}

/**
 * @brief Determines if two coordinates are adjacent on a planet.
 *
 * This function checks if two coordinates on a planet are adjacent to each
 * other. Adjacency is defined as having a maximum difference of 1 in both the x
 * and y coordinates. Additionally, the function handles the case where the
 * coordinates wrap around the planet's boundaries.
 *
 * @param from The starting coordinates.
 * @param to The target coordinates.
 * @return True if the coordinates are adjacent, false otherwise.
 */
bool Planet::is_adjacent(const Coordinates from,
                         const Coordinates to) const noexcept {
  if (std::abs(from.y - to.y) > 1) return false;
  const int dx = std::abs(from.x - to.x);
  if (dx <= 1) return true;
  if (dimensions().x > 0 && dx == dimensions().x - 1) return true;
  return false;
}

std::vector<Coordinates> Planet::adjacent_coordinates(Coordinates from) const {
  std::vector<Coordinates> neighbors;
  if (dimensions().x <= 0 || dimensions().y <= 0) {
    return neighbors;
  }
  neighbors.reserve(8);
  for (int dy = -1; dy <= 1; ++dy) {
    const int new_y = from.y + dy;
    if (new_y < 0 || new_y >= dimensions().y) {
      continue;
    }
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) {
        continue;
      }
      neighbors.push_back(wrap(Coordinates{from.x + dx, new_y}));
    }
  }
  return neighbors;
}

Coordinates Planet::random_adjacent_coordinates(Coordinates from) const {
  const auto neighbors = adjacent_coordinates(from);
  if (neighbors.empty()) {
    return from;
  }
  return neighbors[int_rand(0, neighbors.size() - 1)];
}

void Planet::update_climate(int temp_variance) noexcept {
  conditions(TEMP) = conditions(RTEMP) + temp_variance + int_rand(-5, 5);
}

money_t plinfo::collect_tax(Race::gov& gov, const Race& race) noexcept {
  if (!race.has_government_center()) {
    prod_money = 0;
    return 0;
  }
  prod_money = round_rand(INCOME_FACTOR * static_cast<double>(tax) *
                          static_cast<double>(popn));
  gov.money += prod_money;
  gov.income += prod_money;
  tax += std::min(static_cast<int>(newtax) - static_cast<int>(tax), 5);
  return prod_money;
}

double plinfo::invest_tech(Race::gov& gov, Race& race) noexcept {
  if (!race.has_government_center() || gov.money < tech_invest) {
    prod_tech = 0.0;
    return 0.0;
  }
  prod_tech = tech_prod(static_cast<int>(tech_invest), static_cast<int>(popn));
  gov.money -= tech_invest;
  gov.cost_tech += tech_invest;
  race.tech += prod_tech;
  return prod_tech;
}

void plinfo::update_combat_readiness(long total_mob_points) noexcept {
  mob_points = total_mob_points;
  if (numsectsowned > 0) {
    comread = static_cast<std::uint32_t>(total_mob_points / numsectsowned);
  } else {
    comread = 0;
  }
  guns = static_cast<std::uint32_t>(planet_guns(mob_points));
}
