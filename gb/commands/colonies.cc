// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std.compat;

#include "gb/buffers.h"

module commands;

namespace {
void colonies_at_star(GameObj &g, const Race &race, const starnum_t star) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  stars[star] = getstar(star);
  if (!isset(stars[star].explored(), Playernum)) return;

  for (auto i = 0; i < stars[star].numplanets(); i++) {
    const auto pl = getplanet(star, i);

    if (pl.info[Playernum - 1].explored &&
        pl.info[Playernum - 1].numsectsowned &&
        (!Governor || stars[star].governor(Playernum - 1) == Governor)) {
      sprintf(buf,
              " %c %4.4s/%-4.4s%c%4d%3d%5ld%8ld%3d%6lu%5d%6d "
              "%3d/%-3d%3.0f/%-3d%3d/%-3d",
              Psymbol[pl.type], stars[star].get_name().c_str(),
              stars[star].get_planet_name(i).c_str(),
              (pl.info[Playernum - 1].autorep ? '*' : ' '),
              stars[star].governor(Playernum - 1),
              pl.info[Playernum - 1].numsectsowned,
              pl.info[Playernum - 1].tech_invest, pl.info[Playernum - 1].popn,
              pl.info[Playernum - 1].crystals, pl.info[Playernum - 1].resource,
              pl.info[Playernum - 1].destruct, pl.info[Playernum - 1].fuel,
              pl.info[Playernum - 1].tax, pl.info[Playernum - 1].newtax,
              pl.compatibility(race), pl.conditions[TOXIC],
              pl.info[Playernum - 1].comread, pl.info[Playernum - 1].mob_set);
      notify(Playernum, Governor, buf);
      for (auto j = 1; j <= Num_races; j++)
        if ((j != Playernum) && (pl.info[j - 1].numsectsowned > 0)) {
          sprintf(buf, " %d", j);
          notify(Playernum, Governor, buf);
        }
      g.out << "\n";
    }
  }
}
}  // namespace

namespace GB::commands {
void colonies(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  notify(Playernum, Governor,
         "          ========== Colonization Report ==========\n");
  notify(Playernum, Governor,
         "  Planet     gov sec tech    popn  x   res  "
         "des  fuel  tax  cmpt/tox mob  Aliens\n");

  auto &race = races[Playernum - 1];
  getsdata(&Sdata);

  if (argv.size() < 2)
    for (starnum_t star = 0; star < Sdata.numstars; star++)
      colonies_at_star(g, race, star);
  else
    for (int i = 1; i < argv.size(); i++) {
      Place where{g, argv[i]};
      if (where.err || (where.level == ScopeLevel::LEVEL_UNIV) ||
          (where.level == ScopeLevel::LEVEL_SHIP)) {
        sprintf(buf, "Bad location `%s'.\n", argv[i].c_str());
        notify(Playernum, Governor, buf);
        continue;
      } /* ok, a proper location */
      colonies_at_star(g, race, where.snum);
    }
  g.out << "\n";
}
}  // namespace GB::commands
