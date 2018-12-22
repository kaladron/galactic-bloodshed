// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file mobiliz.c
/// \brief Persuade people to build military stuff.

/*
 *    Sectors that are mobilized produce Destructive Potential in
 *    proportion to the % they are mobilized.  they are also more
 *    damage-resistant.
 */

#include "mobiliz.h"

#include <cstdio>
#include <cstdlib>

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

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "scope must be a planet.\n";
    return;
  }
  if (!control(Playernum, Governor, Stars[g.snum])) {
    g.out << "You are not authorized to do this here.\n";
    return;
  }
  if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1], APcount)) {
    return;
  }

  auto p = getplanet(g.snum, g.pnum);

  auto smap = getsmap(p);

  if (argv.size() < 2) {
    sprintf(buf, "Current mobilization: %d    Quota: %d\n",
            p.info[Playernum - 1].comread, p.info[Playernum - 1].mob_set);
    notify(Playernum, Governor, buf);
    return;
  }
  int sum_mob = std::stoi(argv[1]);

  if (sum_mob > 100 || sum_mob < 0) {
    g.out << "Illegal value.\n";
    return;
  }
  p.info[Playernum - 1].mob_set = sum_mob;
  putplanet(p, Stars[g.snum], g.pnum);
  deductAPs(Playernum, Governor, APcount, g.snum, 0);
}

void tax(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 0;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "scope must be a planet.\n";
    return;
  }
  if (!control(Playernum, Governor, Stars[g.snum])) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  racetype *Race = races[Playernum - 1];
  if (!Race->Gov_ship) {
    g.out << "You have no government center active.\n";
    return;
  }
  if (Race->Guest) {
    g.out << "Sorry, but you can't do this when you are a guest.\n";
    return;
  }
  if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1], APcount)) {
    return;
  }

  auto p = getplanet(g.snum, g.pnum);

  if (argv.size() < 2) {
    sprintf(buf, "Current tax rate: %d%%    Target: %d%%\n",
            p.info[Playernum - 1].tax, p.info[Playernum - 1].newtax);
    notify(Playernum, Governor, buf);
    return;
  }

  int sum_tax = atoi(argv[1].c_str());

  if (sum_tax > 100 || sum_tax < 0) {
    g.out << "Illegal value.\n";
    return;
  }
  p.info[Playernum - 1].newtax = sum_tax;
  putplanet(p, Stars[g.snum], g.pnum);

  deductAPs(Playernum, Governor, APcount, g.snum, 0);
  g.out << "Set.\n";
}

int control(int Playernum, int Governor, startype *star) {
  return (!Governor || star->governor[Playernum - 1] == Governor);
}
