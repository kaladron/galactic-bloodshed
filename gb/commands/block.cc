// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/commands/block.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/power.h"
#include "gb/prof.h"
#include "gb/races.h"
#include "gb/shlmisc.h"
#include "gb/vars.h"
#include "gb/victory.h"

void block(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  player_t p;
  int dummy_;

  auto &race = races[Playernum - 1];

  if (argv.size() == 3 && argv[1] == "player") {
    if (!(p = get_player(argv[2]))) {
      g.out << "No such player.\n";
      return;
    }
    auto &r = races[p - 1];
    dummy_ = 0; /* Used as flag for finding a block */
    sprintf(buf, "Race #%d [%s] is a member of ", p, r.name);
    notify(Playernum, Governor, buf);
    for (int i = 1; i <= Num_races; i++) {
      if (isset(Blocks[i - 1].pledge, p) && isset(Blocks[i - 1].invite, p)) {
        sprintf(buf, "%s%d", (dummy_ == 0) ? " " : ", ", i);
        notify(Playernum, Governor, buf);
        dummy_ = 1;
      }
    }
    if (dummy_ == 0)
      g.out << "no blocks\n";
    else
      g.out << "\n";

    dummy_ = 0; /* Used as flag for finding a block */
    sprintf(buf, "Race #%d [%s] has been invited to join ", p, r.name);
    notify(Playernum, Governor, buf);
    for (int i = 1; i <= Num_races; i++) {
      if (!isset(Blocks[i - 1].pledge, p) && isset(Blocks[i - 1].invite, p)) {
        sprintf(buf, "%s%d", (dummy_ == 0) ? " " : ", ", i);
        notify(Playernum, Governor, buf);
        dummy_ = 1;
      }
    }
    if (dummy_ == 0)
      g.out << "no blocks\n";
    else
      g.out << "\n";

    dummy_ = 0; /* Used as flag for finding a block */
    sprintf(buf, "Race #%d [%s] has pledged ", p, r.name);
    notify(Playernum, Governor, buf);
    for (int i = 1; i <= Num_races; i++) {
      if (isset(Blocks[i - 1].pledge, p) && !isset(Blocks[i - 1].invite, p)) {
        sprintf(buf, "%s%d", (dummy_ == 0) ? " " : ", ", i);
        notify(Playernum, Governor, buf);
        dummy_ = 1;
      }
    }
    if (!dummy_)
      g.out << "no blocks\n";
    else
      g.out << "\n";
  } else if (argv.size() > 1) {
    if (!(p = get_player(argv[1]))) {
      g.out << "No such player,\n";
      return;
    }
    /* list the players who are in this alliance block */
    uint64_t dummy = (Blocks[p - 1].invite & Blocks[p - 1].pledge);
    sprintf(buf, "         ========== %s Power Report ==========\n",
            Blocks[p - 1].name);
    notify(Playernum, Governor, buf);
    sprintf(buf, "         	       %-64.64s\n", Blocks[p - 1].motto);
    notify(Playernum, Governor, buf);
    sprintf(buf,
            "  #  Name              troops  pop  money ship  plan  res "
            "fuel dest know\n");
    notify(Playernum, Governor, buf);

    for (auto i = 1; i <= Num_races; i++)
      if (isset(dummy, i)) {
        auto &r = races[i - 1];
        if (!r.dissolved) {
          sprintf(buf, "%2d %-20.20s ", i, r.name);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].troops, race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].popn, race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].money, race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s",
                  Estimate_i((int)Power[i - 1].ships_owned, race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s",
                  Estimate_i((int)Power[i - 1].planets_owned, race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].resource, race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].fuel, race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].destruct, race, i));
          strcat(buf, temp);
          sprintf(temp, " %3d%%\n", race.translate[i - 1]);
          strcat(buf, temp);
          notify(Playernum, Governor, buf);
        }
      }
  } else { /* list power report for all the alliance blocks (as of the last
              update) */
    sprintf(buf, "         ========== Alliance Blocks as of %s ==========\n",
            std::asctime(std::localtime(&Power_blocks.time)));
    notify(Playernum, Governor, buf);
    sprintf(buf,
            " #  Name             memb money popn ship  sys  res fuel "
            "dest  VPs know\n");
    notify(Playernum, Governor, buf);
    for (auto i = 1; i <= Num_races; i++)
      if (Blocks[i - 1].VPs) {
        sprintf(buf, "%2d %-19.19s%3ld", i, Blocks[i - 1].name,
                Power_blocks.members[i - 1]);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.money[i - 1]), race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.popn[i - 1]), race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.ships_owned[i - 1]), race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.systems_owned[i - 1]), race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.resource[i - 1]), race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.fuel[i - 1]), race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.destruct[i - 1]), race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.VPs[i - 1]), race, i));
        strcat(buf, temp);
        sprintf(temp, " %3d%%\n", race.translate[i - 1]);
        strcat(buf, temp);
        notify(Playernum, Governor, buf);
      }
  }
}
