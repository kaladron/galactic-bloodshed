// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file tax.cc

import gblib;

#include "gb/commands/tax.h"

import std;

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/shlmisc.h"
#include "gb/star.h"
#include "gb/vars.h"

void tax(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 0;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "scope must be a planet.\n";
    return;
  }
  if (!control(*Stars[g.snum], Playernum, Governor)) {
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

  int sum_tax = std::stoi(argv[1]);

  if (sum_tax > 100 || sum_tax < 0) {
    g.out << "Illegal value.\n";
    return;
  }
  p.info[Playernum - 1].newtax = sum_tax;
  putplanet(p, Stars[g.snum], g.pnum);

  deductAPs(Playernum, Governor, APcount, g.snum, 0);
  g.out << "Set.\n";
}
