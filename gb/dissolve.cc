// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* dissolve.c -- commit suicide, nuke all ships and sectors; */

#include "gb/dissolve.h"

#include <cstdio>
#include <cstdlib>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/utils/rand.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/tele.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

void dissolve(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
#ifndef DISSOLVE
  notify(Playernum, Governor,
         "Dissolve has been disabled. Please notify diety.\n");
  return;
#else

  int i;
  int z;
  racetype *Race;
  char racepass[100];
  char govpass[100];

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

  sscanf(argv[1].c_str(), "%s", racepass);
  sscanf(argv[2].c_str(), "%s", govpass);

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
  for (i = 1; i <= n_ships; i++) {
    auto sp = getship(i);
    if (sp->owner != Playernum) continue;
    kill_ship(Playernum, &*sp);
    sprintf(buf, "Ship #%d, self-destruct enabled\n", i);
    notify(Playernum, Governor, buf);
    putship(&*sp);
  }

  getsdata(&Sdata);
  for (z = 0; z < Sdata.numstars; z++) {
    getstar(&(Stars[z]), z);
    if (isset(Stars[z]->explored, Playernum)) {
      for (i = 0; i < Stars[z]->numplanets; i++) {
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

  Race = races[Playernum - 1];
  Race->dissolved = 1;
  putrace(Race);

  sprintf(buf, "%s [%d] has dissolved.\n", Race->name, Playernum);
  post(buf, DECLARATION);

#endif
}

int revolt(Planet *pl, int victim, int agent) {
  int x;
  int y;
  int hix;
  int hiy;
  int lowx;
  int lowy;
  racetype *Race;
  int changed_hands = 0;

  Race = races[victim - 1];

  auto smap = getsmap(*pl);
  /* do the revolt */
  lowx = 0;
  lowy = 0;
  hix = pl->Maxx - 1;
  hiy = pl->Maxy - 1;
  for (y = lowy; y <= hiy; y++) {
    for (x = lowx; x <= hix; x++) {
      auto &s = smap.get(x, y);
      if (s.owner == victim && s.popn) {
        if (success(pl->info[victim - 1].tax)) {
          if (static_cast<unsigned long>(long_rand(1, s.popn)) >
              10 * Race->fighters * s.troops) {
            s.owner = agent;                   /* enemy gets it */
            s.popn = int_rand(1, (int)s.popn); /* some people killed */
            s.troops = 0;                      /* all troops destroyed */
            pl->info[victim - 1].numsectsowned -= 1;
            pl->info[agent - 1].numsectsowned += 1;
            pl->info[victim - 1].mob_points -= s.mobilization;
            pl->info[agent - 1].mob_points += s.mobilization;
            changed_hands++;
          }
        }
      }
    }
  }
  putsmap(smap, *pl);

  return changed_hands;
}
