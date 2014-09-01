// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* dissolve.c -- commit suicide, nuke all ships and sectors; */

#include "dissolve.h"

#include <stdio.h>
#include <stdlib.h>

#include "GB_server.h"
#include "buffers.h"
#include "files.h"
#include "files_shl.h"
#include "races.h"
#include "rand.h"
#include "ships.h"
#include "shlmisc.h"
#include "tele.h"
#include "tweakables.h"
#include "vars.h"

void dissolve(int Playernum, int Governor) {
#ifndef DISSOLVE
  notify(Playernum, Governor,
         "Dissolve has been disabled. Please notify diety.\n");
  return;
#else

  int n_ships;
  int i, j, z, x2, y2, hix, hiy, lowx, lowy;
  unsigned char waste;
  shiptype *sp;
  racetype *Race;
  planettype *pl;
  sectortype *s;
  char nuke;
  char racepass[100], govpass[100];

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
  n_ships = Numships();

  if (argn < 3) {
    sprintf(buf, "Self-Destruct sequence requires passwords.\n");
    notify(Playernum, Governor, buf);
    sprintf(buf,
            "Please use 'dissolve <race password> <leader "
            "password>'<option> to initiate\n");
    notify(Playernum, Governor, buf);
    sprintf(buf, "self-destruct sequence.\n");
    notify(Playernum, Governor, buf);
    return;
  } else {
    sprintf(buf, "WARNING!! WARNING!! WARNING!!\n");
    notify(Playernum, Governor, buf);
    sprintf(buf, "-------------------------------\n");
    notify(Playernum, Governor, buf);
    sprintf(buf, "Entering self destruct sequence!\n");
    notify(Playernum, Governor, buf);

    sscanf(args[1], "%s", racepass);
    sscanf(args[2], "%s", govpass);

    waste = 0;
    if (argn > 3) {
      sscanf(args[3], "%c", &nuke);
      if (nuke == 'w') waste = 1;
    }

    Getracenum(racepass, govpass, &i, &j);

    if (!i || !j) {
      sprintf(buf, "Password mismatch, self-destruct not initiated!\n");
      notify(Playernum, Governor, buf);
      return;
    }

    for (i = 1; i <= n_ships; i++) {
      (void)getship(&sp, i);
      if (sp->owner == Playernum) {
        kill_ship(Playernum, sp);
        sprintf(buf, "Ship #%d, self-destruct enabled\n", i);
        notify(Playernum, Governor, buf);
        putship(sp);
      }
      free(sp);
    }

    getsdata(&Sdata);
    for (z = 0; z < Sdata.numstars; z++) {
      getstar(&(Stars[z]), z);
      if (isset(Stars[z]->explored, Playernum)) {
        for (i = 0; i < Stars[z]->numplanets; i++) {
          getplanet(&pl, z, i);

          if (pl->info[Playernum - 1].explored &&
              pl->info[Playernum - 1].numsectsowned) {
            pl->info[Playernum - 1].fuel = 0;
            pl->info[Playernum - 1].destruct = 0;
            pl->info[Playernum - 1].resource = 0;
            pl->info[Playernum - 1].popn = 0;
            pl->info[Playernum - 1].troops = 0;
            pl->info[Playernum - 1].tax = 0;
            pl->info[Playernum - 1].newtax = 0;
            pl->info[Playernum - 1].crystals = 0;
            pl->info[Playernum - 1].numsectsowned = 0;
            pl->info[Playernum - 1].explored = 0;
            pl->info[Playernum - 1].autorep = 0;
          }

          getsmap(Smap, pl);

          lowx = 0;
          lowy = 0;
          hix = pl->Maxx - 1;
          hiy = pl->Maxy - 1;
          for (y2 = lowy; y2 <= hiy; y2++) {
            for (x2 = lowx; x2 <= hix; x2++) {
              s = &Sector(*pl, x2, y2);
              if (s->owner == Playernum) {
                s->owner = 0;
                s->troops = 0;
                s->popn = 0;
                if (waste) /* code folded from here */
                  s->condition = WASTED;
                /* unfolding */
              }
            }
          }
          putsmap(Smap, pl);
          putstar(Stars[z], z);
          putplanet(pl, z, i);
          free(pl);
        }
      }
    }

    Race = races[Playernum - 1];
    Race->dissolved = 1;
    putrace(Race);

    sprintf(buf, "%s [%d] has dissolved.\n", Race->name, Playernum);
    post(buf, DECLARATION);
  }
#endif
}

int revolt(planettype *pl, int victim, int agent) {
  int x, y, hix, hiy, lowx, lowy;
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
