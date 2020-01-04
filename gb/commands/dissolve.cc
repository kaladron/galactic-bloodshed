// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* dissolve.c -- commit suicide, nuke all ships and sectors; */

import gblib;
import std;

#include "gb/dissolve.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tele.h"
#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

void dissolve(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
#ifndef DISSOLVE
  notify(Playernum, Governor,
         "Dissolve has been disabled. Please notify diety.\n");
  return;
#else

  if (Governor) {
    notify(Playernum, Governor,
           "Only the leader may dissolve the race. The "
           "leader has been notified of your "
           "attempt!!!\n");
    sprintf(buf, "Governor #%d has attempted to dissolve this race.\n",
            Governor);
    notify(Playernum, 0, buf);
    return;
  }

  if (argv.size() < 3) {
    g.out << "Self-Destruct sequence requires passwords.\n";
    g.out << "Please use 'dissolve <race password> <leader "
             "password>'<option> to initiate\n";
    g.out << "self-destruct sequence.\n";
    return;
  }
  g.out << "WARNING!! WARNING!! WARNING!!\n";
  g.out << "-------------------------------\n";
  g.out << "Entering self destruct sequence!\n";

  std::string racepass(argv[1]);
  std::string govpass(argv[2]);

  bool waste = false;
  if (argv.size() > 3) {
    if (argv[3][0] == 'w') waste = true;
  }

  auto [player, governor] = getracenum(racepass, govpass);

  if (!player || !governor) {
    g.out << "Password mismatch, self-destruct not initiated!\n";
    return;
  }

  auto n_ships = Numships();
  for (auto i = 1; i <= n_ships; i++) {
    auto sp = getship(i);
    if (sp->owner != Playernum) continue;
    kill_ship(Playernum, &*sp);
    sprintf(buf, "Ship #%d, self-destruct enabled\n", i);
    notify(Playernum, Governor, buf);
    putship(&*sp);
  }

  getsdata(&Sdata);
  for (auto z = 0; z < Sdata.numstars; z++) {
    getstar(&(Stars[z]), z);
    if (isset(Stars[z]->explored, Playernum)) {
      for (auto i = 0; i < Stars[z]->numplanets; i++) {
        auto pl = getplanet(z, i);

        if (pl.info[Playernum - 1].explored &&
            pl.info[Playernum - 1].numsectsowned) {
          pl.info[Playernum - 1].fuel = 0;
          pl.info[Playernum - 1].destruct = 0;
          pl.info[Playernum - 1].resource = 0;
          pl.info[Playernum - 1].popn = 0;
          pl.info[Playernum - 1].troops = 0;
          pl.info[Playernum - 1].tax = 0;
          pl.info[Playernum - 1].newtax = 0;
          pl.info[Playernum - 1].crystals = 0;
          pl.info[Playernum - 1].numsectsowned = 0;
          pl.info[Playernum - 1].explored = 0;
          pl.info[Playernum - 1].autorep = 0;
        }

        auto smap = getsmap(pl);
        for (auto &s : smap) {
          if (s.owner == Playernum) {
            s.owner = 0;
            s.troops = 0;
            s.popn = 0;
            if (waste) s.condition = SectorType::SEC_WASTED;
          }
        }
        putsmap(smap, pl);
        putstar(Stars[z], z);
        putplanet(pl, Stars[z], i);
      }
    }
  }

  auto Race = races[Playernum - 1];
  Race->dissolved = 1;
  putrace(Race);

  sprintf(buf, "%s [%d] has dissolved.\n", Race->name, Playernum);
  post(buf, DECLARATION);

#endif
}

