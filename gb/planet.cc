// SPDX-License-Identifier: Apache-2.0

module;

import std;

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

  const auto* victim_race = entity_manager.peek_race(victim);
  if (!victim_race) return 0;

  auto smap_handle = entity_manager.get_sectormap(snum, pnum);
  auto& smap = *smap_handle;
  for (auto& s : smap) {
    if (s.get_owner() != victim || s.get_popn() == 0) continue;

    // Revolt rate is a function of tax rate.
    if (!success(pl.info(victim).tax)) continue;

    if (long_rand(1, s.get_popn()) <=
        10L * victim_race->fighters * s.get_troops())
      continue;

    // Revolt successful.
    s.set_owner(agent);                              /* enemy gets it */
    s.subtract_popn(long_rand(0, s.get_popn() - 1)); /* some people killed */
    s.set_troops(0);                                 /* all troops destroyed */
    pl.info(victim).numsectsowned -= 1;
    pl.info(agent).numsectsowned += 1;
    pl.info(victim).mob_points -= s.get_mobilization();
    pl.info(agent).mob_points += s.get_mobilization();
    revolted_sectors++;
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
 * displacement. Turn statistics and system inhabited status are updated as
 * part of the movement process.
 *
 * @param entity_manager The entity manager for accessing star and ship data.
 * @param starnum The unique identifier of the star system.
 * @param planet The planet object to be moved.
 * @param planetnum The index of the planet within its star system.
 * @param stats Structure for tracking turn-based statistics.
 */
void moveplanet(EntityManager& entity_manager, const starnum_t starnum,
                Planet& planet, const planetnum_t planetnum, TurnStats& stats) {
  if (planet.popn() || planet.ships()) {
    stats.Stinfo[starnum][planetnum].inhab = 1;
  }

  auto star_handle = entity_manager.get_star(starnum);
  auto& star = *star_handle;
  stats.StarsInhab[starnum] = !!(star.inhabited());
  stats.StarsExpl[starnum] = !!(star.explored());

  star.inhabited() = 0;

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
 * @param p The planet object.
 * @param from The starting coordinates.
 * @param to The target coordinates.
 * @return True if the coordinates are adjacent, false otherwise.
 */
bool adjacent(const Planet& p, const Coordinates from, const Coordinates to) {
  if (std::abs(from.y - to.y) > 1) return false;
  if (std::abs(from.x - to.x) <= 1) return true;
  if (from.x == p.Maxx() - 1 && to.x == 0) return true;
  if (from.x == 0 && to.x == p.Maxx() - 1) return true;
  return false;
}
