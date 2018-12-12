// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*
 * mobiliz.c -- persuade people to build military stuff.
 *    Sectors that are mobilized produce Destructive Potential in
 *    proportion to the % they are mobilized.  they are also more
 *    damage-resistant.
 */

#include "mobiliz.h"

#include <stdio.h>
#include <stdlib.h>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "races.h"
#include "shlmisc.h"
#include "vars.h"

void mobilize(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 1;
  int sum_mob = 0;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    sprintf(buf, "scope must be a planet.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (!control(Playernum, Governor, Stars[Dir[Playernum - 1][Governor].snum])) {
    notify(Playernum, Governor, "You are not authorized to do this here.\n");
    return;
  }
  if (!enufAP(Playernum, Governor,
              Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
              APcount)) {
    return;
  }

  auto p = getplanet(Dir[Playernum - 1][Governor].snum,
                     Dir[Playernum - 1][Governor].pnum);

  auto smap = getsmap(p);

  if (argn < 2) {
    sprintf(buf, "Current mobilization: %d    Quota: %d\n",
            p.info[Playernum - 1].comread, p.info[Playernum - 1].mob_set);
    notify(Playernum, Governor, buf);
    return;
  }
  sum_mob = atoi(args[1]);

  if (sum_mob > 100 || sum_mob < 0) {
    sprintf(buf, "Illegal value.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  p.info[Playernum - 1].mob_set = sum_mob;
  putplanet(p, Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].pnum);
  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);
}

void tax(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 0;
  int sum_tax = 0;
  racetype *Race;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    sprintf(buf, "scope must be a planet.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (!control(Playernum, Governor, Stars[Dir[Playernum - 1][Governor].snum])) {
    notify(Playernum, Governor, "You are not authorized to do that here.\n");
    return;
  }
  Race = races[Playernum - 1];
  if (!Race->Gov_ship) {
    notify(Playernum, Governor, "You have no government center active.\n");
    return;
  }
  if (Race->Guest) {
    notify(Playernum, Governor,
           "Sorry, but you can't do this when you are a guest.\n");
    return;
  }
  if (!enufAP(Playernum, Governor,
              Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
              APcount)) {
    return;
  }

  auto p = getplanet(Dir[Playernum - 1][Governor].snum,
                     Dir[Playernum - 1][Governor].pnum);

  if (argn < 2) {
    sprintf(buf, "Current tax rate: %d%%    Target: %d%%\n",
            p.info[Playernum - 1].tax, p.info[Playernum - 1].newtax);
    notify(Playernum, Governor, buf);
    return;
  }

  sum_tax = atoi(args[1]);

  if (sum_tax > 100 || sum_tax < 0) {
    sprintf(buf, "Illegal value.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  p.info[Playernum - 1].newtax = sum_tax;
  putplanet(p, Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].pnum);

  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);
  notify(Playernum, Governor, "Set.\n");
}

int control(int Playernum, int Governor, startype *star) {
  return (!Governor || star->governor[Playernum - 1] == Governor);
}
