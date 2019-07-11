// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* capital.c -- designate a capital */

#include "gb/capital.h"

#include <cstdio>
#include <cstdlib>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/fire.h"
#include "gb/getplace.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"
#include "gb/vars.h"

void capital(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 50;
  racetype *Race;

  Race = races[Playernum - 1];
  if (Governor) {
    g.out << "Only the leader may designate the capital.\n";
    return;
  }

  shipnum_t shipno;
  if (argv.size() != 2)
    shipno = Race->Gov_ship;
  else {
    auto shiptmp = string_to_shipnum(argv[1]);
    if (!shiptmp) {
      g.out << "Specify a valid ship number.\n";
      return;
    }
    shipno = *shiptmp;
  }

  auto s = getship(shipno);
  if (!s) {
    g.out << "Change the capital to be what ship?\n";
    return;
  }

  if (argv.size() == 2) {
    shipnum_t snum = s->storbits;
    if (testship(Playernum, Governor, &*s)) {
      g.out << "You can't do that!\n";
      return;
    }
    if (!landed(*s)) {
      g.out << "Try landing this ship first!\n";
      return;
    }
    if (!enufAP(Playernum, Governor, Stars[snum]->AP[Playernum - 1], APcount)) {
      return;
    }
    if (s->type != ShipType::OTYPE_GOV) {
      sprintf(buf, "That ship is not a %s.\n", Shipnames[ShipType::OTYPE_GOV]);
      notify(Playernum, Governor, buf);
      return;
    }
    deductAPs(Playernum, Governor, APcount, snum, 0);
    Race->Gov_ship = shipno;
    putrace(Race);
  }

  sprintf(
      buf, "Efficiency of governmental center: %.0f%%.\n",
      ((double)s->popn / (double)Max_crew(&*s)) * (100 - (double)s->damage));
  notify(Playernum, Governor, buf);
}
