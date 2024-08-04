// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/GB_server.h"
#include "gb/buffers.h"

module commands;

void transfer(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;
  player_t player;
  char commod = 0;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    sprintf(buf, "You need to be in planet scope to do this.\n");
    notify(Playernum, Governor, buf);
    return;
  }

  if (!enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1], APcount))
    return;

  if (!(player = get_player(argv[1]))) {
    sprintf(buf, "No such player.\n");
    notify(Playernum, Governor, buf);
    return;
  }

  auto planet = getplanet(g.snum, g.pnum);

  sscanf(argv[2].c_str(), "%c", &commod);
  // TODO(jeffbailey): May throw an exception on a negative number.
  resource_t give = std::stoul(argv[3]);

  sprintf(temp, "%s/%s:", stars[g.snum].name, stars[g.snum].pnames[g.pnum]);
  switch (commod) {
    case 'r':
      if (give > planet.info[Playernum - 1].resource) {
        sprintf(buf, "You don't have %lu on this planet.\n", give);
        notify(Playernum, Governor, buf);
      } else {
        planet.info[Playernum - 1].resource -= give;
        planet.info[player - 1].resource += give;
        sprintf(buf,
                "%s %lu resources transferred from player %d to player #%d\n",
                temp, give, Playernum, player);
        notify(Playernum, Governor, buf);
        warn_race(player, buf);
      }
      break;
    case 'x':
    case '&':
      if (give > planet.info[Playernum - 1].crystals) {
        sprintf(buf, "You don't have %lu on this planet.\n", give);
        notify(Playernum, Governor, buf);
      } else {
        planet.info[Playernum - 1].crystals -= give;
        planet.info[player - 1].crystals += give;
        sprintf(buf,
                "%s %lu crystal(s) transferred from player %d to player #%d\n",
                temp, give, Playernum, player);
        notify(Playernum, Governor, buf);
        warn_race(player, buf);
      }
      break;
    case 'f':
      if (give > planet.info[Playernum - 1].fuel) {
        sprintf(buf, "You don't have %lu fuel on this planet.\n", give);
        notify(Playernum, Governor, buf);
      } else {
        planet.info[Playernum - 1].fuel -= give;
        planet.info[player - 1].fuel += give;
        sprintf(buf, "%s %lu fuel transferred from player %d to player #%d\n",
                temp, give, Playernum, player);
        notify(Playernum, Governor, buf);
        warn_race(player, buf);
      }
      break;
    case 'd':
      if (give > planet.info[Playernum - 1].destruct) {
        sprintf(buf, "You don't have %lu destruct on this planet.\n", give);
        notify(Playernum, Governor, buf);
      } else {
        planet.info[Playernum - 1].destruct -= give;
        planet.info[player - 1].destruct += give;
        sprintf(buf,
                "%s %lu destruct transferred from player %d to player #%d\n",
                temp, give, Playernum, player);
        notify(Playernum, Governor, buf);
        warn_race(player, buf);
      }
      break;
    default:
      sprintf(buf, "What?\n");
      notify(Playernum, Governor, buf);
  }

  putplanet(planet, stars[g.snum], g.pnum);

  deductAPs(g, APcount, g.snum);
}