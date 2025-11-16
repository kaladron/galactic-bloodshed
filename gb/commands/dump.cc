// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void dump(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 10;
  player_t player;
  int star;
  int j;

  if (!enufAP(Playernum, Governor, stars[g.snum].AP(Playernum - 1), APcount))
    return;

  if (!(player = get_player(argv[1]))) {
    notify(Playernum, Governor, "No such player.\n");
    return;
  }

  /* transfer all planet and star knowledge to the player */
  /* get all stars and planets */
  auto &race = races[Playernum - 1];
  if (race.Guest) {
    g.out << "Cheater!\n";
    return;
  }
  if (Governor) {
    g.out << "Only leaders are allowed to use dump.\n";
    return;
  }
  getsdata(&Sdata);

  if (argv.size() < 3) {
    for (star = 0; star < Sdata.numstars; star++) {
      stars[star] = getstar(star);

      if (isset(stars[star].explored(), Playernum)) {
        setbit(stars[star].explored(), player);

        for (size_t i = 0; i < stars[star].numplanets(); i++) {
          auto planet = getplanet(star, i);
          if (planet.info(Playernum - 1).explored) {
            planet.info(player - 1).explored = 1;
            putplanet(planet, stars[star], i);
          }
        }
        putstar(stars[star], star);
      }
    }
  } else { /* list of places given */
    for (size_t i = 2; i < argv.size(); i++) {
      Place where{g, argv[i], true};
      if (!where.err && where.level != ScopeLevel::LEVEL_UNIV &&
          where.level != ScopeLevel::LEVEL_SHIP) {
        star = where.snum;
        stars[star] = getstar(star);

        if (isset(stars[star].explored(), Playernum)) {
          setbit(stars[star].explored(), player);

          for (j = 0; j < stars[star].numplanets(); j++) {
            auto planet = getplanet(star, j);
            if (planet.info(Playernum - 1).explored) {
              planet.info(player - 1).explored = 1;
              putplanet(planet, stars[star], j);
            }
          }
          putstar(stars[star], star);
        }
      }
    }
  }

  deductAPs(g, APcount, g.snum);

  warn_race(player, std::format("{} [{}] has given you exploration data.\n",
                                race.name, Playernum));
  g.out << "Exploration Data transferred.\n";
}
}  // namespace GB::commands
