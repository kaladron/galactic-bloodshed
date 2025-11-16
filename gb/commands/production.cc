// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std.compat;

module commands;

namespace {
void production_at_star(GameObj &g, starnum_t star) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;

  const auto* star_ptr = g.entity_manager.peek_star(star);
  if (!star_ptr) return;
  if (!isset(star_ptr->explored(), Playernum)) return;

  for (auto i = 0; i < star_ptr->numplanets(); i++) {
    const auto* pl = g.entity_manager.peek_planet(star, i);
    if (!pl) continue;

    if (pl->info(Playernum - 1).explored &&
        pl->info(Playernum - 1).numsectsowned &&
        (!Governor || star_ptr->governor(Playernum - 1) == Governor)) {
      const auto star4 = std::string(star_ptr->get_name()).substr(0, 4);
      const auto planet4 = std::string(star_ptr->get_planet_name(i)).substr(0, 4);
      notify(
          Playernum, Governor,
          std::format(" {} {:>4}/{:<4}{}{:>3}{:>8.4f}{:>8}{:>3}{:>6}{:>5}{:>6} "
                      "{:>6}   {:>3}{:>8.2f}\n",
                      Psymbol[pl->type()], star4, planet4,
                      (pl->info(Playernum - 1).autorep ? '*' : ' '),
                      star_ptr->governor(Playernum - 1),
                      pl->info(Playernum - 1).prod_tech, pl->total_resources(),
                      pl->info(Playernum - 1).prod_crystals,
                      pl->info(Playernum - 1).prod_res,
                      pl->info(Playernum - 1).prod_dest,
                      pl->info(Playernum - 1).prod_fuel,
                      pl->info(Playernum - 1).prod_money,
                      pl->info(Playernum - 1).tox_thresh,
                      pl->info(Playernum - 1).est_production));
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
        notify(Playernum, Governor,
               std::format("Bad location `{}`.\n", argv[i]));
        continue;
      } /* ok, a proper location */
      production_at_star(g, where.snum);
    }
  g.out << "\n";
}
}  // namespace GB::commands
