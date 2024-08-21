// SPDX-License-Identifier: Apache-2.0

module;

import std.compat;

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

    if (static_cast<unsigned long>(long_rand(1, s.popn)) <=
        10 * races[victim - 1].fighters * s.troops)
      continue;

    // Revolt successful.
    s.owner = agent;                   /* enemy gets it */
    s.popn = int_rand(1, (int)s.popn); /* some people killed */
    s.troops = 0;                      /* all troops destroyed */
    pl.info[victim - 1].numsectsowned -= 1;
    pl.info[agent - 1].numsectsowned += 1;
    pl.info[victim - 1].mob_points -= s.mobilization;
    pl.info[agent - 1].mob_points += s.mobilization;
    revolted_sectors++;
  }
  putsmap(smap, pl);

  return revolted_sectors;
}
