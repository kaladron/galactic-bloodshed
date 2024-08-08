// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* declare.c -- declare alliance, neutrality, war, the basic thing. */

module;

import gblib;
import std.compat;

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/races.h"
#include "gb/tele.h"
#include "gb/tweakables.h"

module commands;

namespace GB::commands {
void declare(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  const ap_t APcount = 1;
  player_t n;
  int d_mod;

  if (Governor) {
    g.out << "Only leaders may declare.\n";
    return;
  }

  if (!(n = get_player(argv[1]))) {
    g.out << "No such player.\n";
    return;
  }

  /* look in sdata for APs first */
  /* enufAPs would print something */
  if ((int)Sdata.AP[Playernum - 1] >= APcount) {
    deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
    /* otherwise use current star */
  } else if ((g.level == ScopeLevel::LEVEL_STAR ||
              g.level == ScopeLevel::LEVEL_PLAN) &&
             enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1],
                    APcount)) {
    deductAPs(g, APcount, g.snum);
  } else {
    sprintf(buf, "You don't have enough AP's (%d)\n", APcount);
    notify(Playernum, Governor, buf);
    return;
  }

  auto& race = races[Playernum - 1];
  auto& alien = races[n - 1];

  switch (*argv[2].c_str()) {
    case 'a':
      setbit(race.allied, n);
      clrbit(race.atwar, n);
      if (success(5)) {
        sprintf(buf, "But would you want your sister to marry one?\n");
        notify(Playernum, Governor, buf);
      } else {
        sprintf(buf, "Good for you.\n");
        notify(Playernum, Governor, buf);
      }
      sprintf(buf, " Player #%d (%s) has declared an alliance with you!\n",
              Playernum, race.name);
      warn_race(n, buf);
      sprintf(buf, "%s [%d] declares ALLIANCE with %s [%d].\n", race.name,
              Playernum, alien.name, n);
      d_mod = 30;
      if (argv.size() > 3) d_mod = std::stoi(argv[3]);
      d_mod = std::max(d_mod, 30);
      break;
    case 'n':
      clrbit(race.allied, n);
      clrbit(race.atwar, n);
      sprintf(buf, "Done.\n");
      notify(Playernum, Governor, buf);

      sprintf(buf, " Player #%d (%s) has declared neutrality with you!\n",
              Playernum, race.name);
      warn_race(n, buf);
      sprintf(buf, "%s [%d] declares a state of neutrality with %s [%d].\n",
              race.name, Playernum, alien.name, n);
      d_mod = 30;
      break;
    case 'w':
      setbit(race.atwar, n);
      clrbit(race.allied, n);
      if (success(4)) {
        sprintf(buf,
                "Your enemies flaunt their secondary male reproductive "
                "glands in your\ngeneral direction.\n");
        notify(Playernum, Governor, buf);
      } else {
        sprintf(buf, "Give 'em hell!\n");
        notify(Playernum, Governor, buf);
      }
      sprintf(buf, " Player #%d (%s) has declared war against you!\n",
              Playernum, race.name);
      warn_race(n, buf);
      switch (int_rand(1, 5)) {
        case 1:
          sprintf(buf, "%s [%d] declares WAR on %s [%d].\n", race.name,
                  Playernum, alien.name, n);
          break;
        case 2:
          sprintf(buf, "%s [%d] has had enough of %s [%d] and declares WAR!\n",
                  race.name, Playernum, alien.name, n);
          break;
        case 3:
          sprintf(
              buf,
              "%s [%d] decided that it is time to declare WAR on %s [%d]!\n",
              race.name, Playernum, alien.name, n);
          break;
        case 4:
          sprintf(buf,
                  "%s [%d] had no choice but to declare WAR against %s [%d]!\n",
                  race.name, Playernum, alien.name, n);
          break;
        case 5:
          sprintf(buf,
                  "%s [%d] says 'screw it!' and declares WAR on %s [%d]!\n",
                  race.name, Playernum, alien.name, n);
          break;
        default:
          break;
      }
      d_mod = 30;
      break;
    default:
      g.out << "I don't understand.\n";
      return;
  }

  post(buf, DECLARATION);
  warn_race(Playernum, buf);

  /* They, of course, learn more about you */
  alien.translate[Playernum - 1] =
      MIN(alien.translate[Playernum - 1] + d_mod, 100);

  putrace(alien);
  putrace(race);
}
}  // namespace GB::commands
