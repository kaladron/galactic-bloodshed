// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* power.c -- display power report */

#include "powercmd.h"

#include <cstdio>
#include <cstring>

#include "GB_server.h"
#include "buffers.h"
#include "power.h"
#include "prof.h"
#include "races.h"
#include "shlmisc.h"
#include "vars.h"
#include "victory.h"

static void prepare_output_line(Race *, Race *, int, int);

void block(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  player_t p;
  racetype *r;
  racetype *Race;
  int dummy_;
  int dummy[2];

  Race = races[Playernum - 1];

  if (argv.size() == 3 && argv[1] == "player") {
    if (!(p = get_player(argv[2]))) {
      g.out << "No such player.\n";
      return;
    }
    r = races[p - 1];
    dummy_ = 0; /* Used as flag for finding a block */
    sprintf(buf, "Race #%d [%s] is a member of ", p, r->name);
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
    sprintf(buf, "Race #%d [%s] has been invited to join ", p, r->name);
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
    sprintf(buf, "Race #%d [%s] has pledged ", p, r->name);
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
    r = races[p - 1];
    /* list the players who are in this alliance block */
    dummy[0] = (Blocks[p - 1].invite[0] & Blocks[p - 1].pledge[0]);
    dummy[1] = (Blocks[p - 1].invite[1] & Blocks[p - 1].pledge[1]);
    sprintf(buf, "         ========== %s Power Report ==========\n",
            Blocks[p - 1].name);
    notify(Playernum, Governor, buf);
    sprintf(buf, "         	       %-64.64s\n", Blocks[p - 1].motto);
    notify(Playernum, Governor, buf);
    sprintf(buf,
            "  #  Name              troops  pop  money ship  plan  res "
            "fuel dest know\n");
    notify(Playernum, Governor, buf);

    for (int i = 1; i <= Num_races; i++)
      if (isset(dummy, i)) {
        r = races[i - 1];
        if (!r->dissolved) {
          sprintf(buf, "%2d %-20.20s ", i, r->name);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].troops, Race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].popn, Race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].money, Race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s",
                  Estimate_i((int)Power[i - 1].ships_owned, Race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s",
                  Estimate_i((int)Power[i - 1].planets_owned, Race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].resource, Race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].fuel, Race, i));
          strcat(buf, temp);
          sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].destruct, Race, i));
          strcat(buf, temp);
          sprintf(temp, " %3d%%\n", Race->translate[i - 1]);
          strcat(buf, temp);
          notify(Playernum, Governor, buf);
        }
      }
  } else { /* list power report for all the alliance blocks (as of the last
              update) */
    sprintf(buf, "         ========== Alliance Blocks as of %s ==========\n",
            Power_blocks.time);
    notify(Playernum, Governor, buf);
    sprintf(buf,
            " #  Name             memb money popn ship  sys  res fuel "
            "dest  VPs know\n");
    notify(Playernum, Governor, buf);
    for (int i = 1; i <= Num_races; i++)
      if (Blocks[i - 1].VPs) {
        sprintf(buf, "%2d %-19.19s%3ld", i, Blocks[i - 1].name,
                Power_blocks.members[i - 1]);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.money[i - 1]), Race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.popn[i - 1]), Race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.ships_owned[i - 1]), Race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.systems_owned[i - 1]), Race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.resource[i - 1]), Race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.fuel[i - 1]), Race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.destruct[i - 1]), Race, i));
        strcat(buf, temp);
        sprintf(temp, "%5s",
                Estimate_i((int)(Power_blocks.VPs[i - 1]), Race, i));
        strcat(buf, temp);
        sprintf(temp, " %3d%%\n", Race->translate[i - 1]);
        strcat(buf, temp);
        notify(Playernum, Governor, buf);
      }
  }
}

void power(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): int APcount = 0;
  player_t p;
  racetype *r;
  racetype *Race;
  struct vic vic[MAXPLAYERS];

  p = -1;

  if (argv.size() >= 2) {
    if (!(p = get_player(argv[1]))) {
      g.out << "No such player,\n";
      return;
    }
    r = races[p - 1];
  }

  Race = races[Playernum - 1];

  sprintf(buf,
          "         ========== Galactic Bloodshed Power Report ==========\n");
  notify(Playernum, Governor, buf);

  if (Race->God)
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
    create_victory_list(vic);
    for (int i = 1; i <= Num_races; i++) {
      p = vic[i - 1].racenum;
      r = races[p - 1];
      if (!r->dissolved && Race->translate[p - 1] >= 10) {
        prepare_output_line(Race, r, p, i);
        notify(Playernum, Governor, buf);
      }
    }
  } else {
    r = races[p - 1];
    prepare_output_line(Race, r, p, 0);
    notify(Playernum, Governor, buf);
  }
}

static void prepare_output_line(racetype *Race, racetype *r, int i, int rank) {
  if (rank)
    sprintf(buf, "%2d ", rank);
  else
    buf[0] = '\0';
  sprintf(temp, "[%2d]%s%s%-15.15s %5s", i,
          isset(Race->allied, i) ? "+" : (isset(Race->atwar, i) ? "-" : " "),
          isset(r->allied, Race->Playernum)
              ? "+"
              : (isset(r->atwar, Race->Playernum) ? "-" : " "),
          r->name, Estimate_i((int)r->victory_score, Race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].troops, Race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].popn, Race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].money, Race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].ships_owned, Race, i));
  strcat(buf, temp);
  sprintf(temp, "%3s", Estimate_i((int)Power[i - 1].planets_owned, Race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].resource, Race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].fuel, Race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)Power[i - 1].destruct, Race, i));
  strcat(buf, temp);
  sprintf(temp, "%5s", Estimate_i((int)r->morale, Race, i));
  strcat(buf, temp);
  if (Race->God)
    sprintf(temp, " %3d\n", Sdata.VN_hitlist[i - 1]);
  else
    sprintf(temp, " %3d%%\n", Race->translate[i - 1]);
  strcat(buf, temp);
}
