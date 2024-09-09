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
int revolt(Planet &pl, const player_t victim, const player_t agent) {
  int revolted_sectors = 0;

  auto smap = getsmap(pl);
  for (auto &s : smap) {
    if (s.owner != victim || s.popn == 0) continue;

    // Revolt rate is a function of tax rate.
    if (!success(pl.info[victim - 1].tax)) continue;

    if (long_rand(1, s.popn) <= 10L * races[victim - 1].fighters * s.troops)
      continue;

    // Revolt successful.
    s.owner = agent;               /* enemy gets it */
    s.popn = long_rand(1, s.popn); /* some people killed */
    s.troops = 0;                  /* all troops destroyed */
    pl.info[victim - 1].numsectsowned -= 1;
    pl.info[agent - 1].numsectsowned += 1;
    pl.info[victim - 1].mob_points -= s.mobilization;
    pl.info[agent - 1].mob_points += s.mobilization;
    revolted_sectors++;
  }
  putsmap(smap, pl);

  return revolted_sectors;
}

void moveplanet(const starnum_t starnum, Planet &planet,
                const planetnum_t planetnum) {
  if (planet.popn || planet.ships) Stinfo[starnum][planetnum].inhab = 1;

  StarsInhab[starnum] = !!(stars[starnum].inhabited);
  StarsExpl[starnum] = !!(stars[starnum].explored);

  stars[starnum].inhabited = 0;
  if (!StarsExpl[starnum]) return; /* no one's explored the star yet */

  double dist = std::hypot((double)(planet.ypos), (double)(planet.xpos));

  double phase = std::atan2((double)(planet.ypos), (double)(planet.xpos));
  double period =
      dist *
      std::sqrt((double)(dist / (SYSTEMGRAVCONST * stars[starnum].gravity)));
  /* keppler's law */

  double xadd = dist * std::cos((double)(-1. / period + phase)) - planet.xpos;
  double yadd = dist * std::sin((double)(-1. / period + phase)) - planet.ypos;
  /* one update time unit - planets orbit counter-clockwise */

  /* adjust ships in orbit around the planet */
  auto sh = planet.ships;
  while (sh) {
    auto ship = ships[sh];
    ship->xpos += xadd;
    ship->ypos += yadd;
    sh = ship->nextship;
  }

  planet.xpos += xadd;
  planet.ypos += yadd;
}
