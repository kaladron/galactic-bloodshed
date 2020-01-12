// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* power.c -- display power report */

import gblib;
import std;

#include "gb/commands/power.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/power.h"
#include "gb/prof.h"
#include "gb/races.h"
#include "gb/shlmisc.h"
#include "gb/vars.h"
#include "gb/victory.h"

namespace {
void prepare_output_line(const Race &race, const Race &r, int i, int rank) {
  if (rank != 0)
    sprintf(buf, "%2d ", rank);
  else
    buf[0] = '\0';
  sprintf(temp, "[%2d]%s%s%-15.15s %5s", i,
          isset(race.allied, i) ? "+" : (isset(race.atwar, i) ? "-" : " "),
          isset(r.allied, race.Playernum)
              ? "+"
              : (isset(r.atwar, race.Playernum) ? "-" : " "),
          r.name, Estimate_i((int)r.victory_score, race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].troops, race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].popn, race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].money, race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].ships_owned, race, i));
  strcat(buf, temp);
  sprintf(temp, "%3s", Estimate_i((int)Power[i - 1].planets_owned, race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].resource, race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].fuel, race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].destruct, race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)r.morale, race, i));
  strcat(buf, temp);
  if (race.God)
    sprintf(temp, " %3d\n", Sdata.VN_hitlist[i - 1]);
  else
    sprintf(temp, " %3d%%\n", race.translate[i - 1]);
  strcat(buf, temp);
}
}  // namespace

void power(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  player_t p = -1;

  if (argv.size() >= 2) {
    if (!(p = get_player(argv[1]))) {
      g.out << "No such player,\n";
      return;
    }
  }

  auto &race = races[Playernum - 1];

  sprintf(buf,
          "         ========== Galactic Bloodshed Power Report ==========\n");
  notify(Playernum, Governor, buf);

  if (race.God)
    sprintf(buf,
            "%s  #  Name               VP  mil  civ cash ship pl  res "
            "fuel dest morl VNs\n",
            argv.size() < 2 ? "rank" : "");
  else
    sprintf(buf,
            "%s  #  Name               VP  mil  civ cash ship pl  res "
            "fuel dest morl know\n",
            argv.size() < 2 ? "rank" : "");
  notify(Playernum, Governor, buf);

  if (argv.size() < 2) {
    auto vicvec = create_victory_list();
    int rank = 0;
    for (const auto &vic : vicvec) {
      rank++;
      p = vic.racenum;
      auto &r = races[p - 1];
      if (!r.dissolved && race.translate[p - 1] >= 10) {
        prepare_output_line(race, r, p, rank);
        notify(Playernum, Governor, buf);
      }
    }
  } else {
    auto &r = races[p - 1];
    prepare_output_line(race, r, p, 0);
    notify(Playernum, Governor, buf);
  }
}
