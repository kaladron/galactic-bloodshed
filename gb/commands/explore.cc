// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void explore(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int starq;
  int j;

  starq = -1;

  if (argv.size() == 2) {
    Place where{g, argv[1]};
    if (where.err) {
      notify(Playernum, Governor, "explore: bad scope.\n");
      return;
    }
    if (where.level == ScopeLevel::LEVEL_SHIP ||
        where.level == ScopeLevel::LEVEL_UNIV) {
      notify(Playernum, Governor, std::format("Bad scope '{}'\n", argv[1]));
      return;
    }
    starq = where.snum;
  }

  auto& race = races[Playernum - 1];

  getsdata(&Sdata);
  notify(Playernum, Governor,
         "         ========== Exploration Report ==========\n");
  notify(
      Playernum, Governor,
      std::format(" Global action points : [{:2}]\n", Sdata.AP[Playernum - 1]));
  notify(
      Playernum, Governor,
      " Star  (stability)[AP]   #  Planet [Attributes] Type (Compatibility)\n");
  for (starnum_t star = 0; star < Sdata.numstars; star++)
    if ((starq == -1) || (starq == star)) {
      const auto* star_ptr = g.entity_manager.peek_star(star);
      if (!star_ptr) continue;
      
      if (isset(star_ptr->explored(), Playernum))
        for (planetnum_t i = 0; i < star_ptr->numplanets(); i++) {
          const auto* pl = g.entity_manager.peek_planet(star, i);
          if (!pl) continue;
          
          if (i == 0) {
            if (race.tech >= TECH_SEE_STABILITY) {
              notify(
                  Playernum, Governor,
                  std::format("\n{:13} ({:2})[{:2}]\n", star_ptr->get_name(),
                              star_ptr->stability(),
                              star_ptr->AP(Playernum - 1)));
            } else {
              notify(
                  Playernum, Governor,
                  std::format("\n{:13} (/?/?)[{:2}]\n", star_ptr->get_name(),
                              star_ptr->AP(Playernum - 1)));
            }
          }

          notify(Playernum, Governor, "\t\t      ");

          notify(Playernum, Governor,
                 std::format("  #{}. {:<15} [ ", i + 1,
                             star_ptr->get_planet_name(i)));
          if (pl->info(Playernum - 1).explored) {
            notify(Playernum, Governor, "Ex ");
            if (pl->info(Playernum - 1).autorep) {
              notify(Playernum, Governor, "Rep ");
            }
            if (pl->info(Playernum - 1).numsectsowned) {
              notify(Playernum, Governor, "Inhab ");
            }
            if (pl->slaved_to()) {
              notify(Playernum, Governor, "SLAVED ");
            }
            for (j = 1; j <= Num_races; j++)
              if (j != Playernum && pl->info(j - 1).numsectsowned) {
                notify(Playernum, Governor, std::format("{} ", j));
              }
            if (pl->conditions(TOXIC) > 70) {
              notify(Playernum, Governor, "TOXIC ");
            }
            notify(Playernum, Governor,
                   std::format("] {} {:2.0f}%\n", Planet_types[pl->type()],
                               pl->compatibility(race)));
          } else {
            notify(Playernum, Governor, "No Data ]\n");
          }
        }
    }
}
}  // namespace GB::commands
