// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std.compat;

#include "gb/buffers.h"

module commands;

namespace {
void production_at_star(GameObj &g, starnum_t star) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  stars[star] = getstar(star);
  if (!isset(stars[star].explored(), Playernum)) return;

  for (auto i = 0; i < stars[star].numplanets(); i++) {
    const auto pl = getplanet(star, i);

    if (pl.info[Playernum - 1].explored &&
        pl.info[Playernum - 1].numsectsowned &&
        (!Governor || stars[star].governor(Playernum - 1) == Governor)) {
      sprintf(
          buf, " %c %4.4s/%-4.4s%c%3d%8.4f%8ld%3d%6d%5d%6d %6ld   %3d%8.2f\n",
          Psymbol[pl.type], stars[star].get_name().c_str(),
          stars[star].get_planet_name(i).c_str(),
          (pl.info[Playernum - 1].autorep ? '*' : ' '),
          stars[star].governor(Playernum - 1), pl.info[Playernum - 1].prod_tech,
          pl.total_resources, pl.info[Playernum - 1].prod_crystals,
          pl.info[Playernum - 1].prod_res, pl.info[Playernum - 1].prod_dest,
          pl.info[Playernum - 1].prod_fuel, pl.info[Playernum - 1].prod_money,
          pl.info[Playernum - 1].tox_thresh,
          pl.info[Playernum - 1].est_production);
      notify(Playernum, Governor, buf);
    }
  }
}
}  // namespace

namespace GB::commands {
void production(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  notify(Playernum, Governor,
         "          ============ Production Report ==========\n");
  notify(Playernum, Governor,
         "  Planet     gov    tech deposit  x   res  "
         "des  fuel    tax   tox  est prod\n");

  getsdata(&Sdata);

  if (argv.size() < 2)
    for (starnum_t star = 0; star < Sdata.numstars; star++)
      production_at_star(g, star);
  else
    for (int i = 1; i < argv.size(); i++) {
      Place where{g, argv[i]};
      if (where.err || (where.level == ScopeLevel::LEVEL_UNIV) ||
          (where.level == ScopeLevel::LEVEL_SHIP)) {
        sprintf(buf, "Bad location `%s'.\n", argv[i].c_str());
        notify(Playernum, Governor, buf);
        continue;
      } /* ok, a proper location */
      production_at_star(g, where.snum);
    }
  g.out << "\n";
}
}  // namespace GB::commands
