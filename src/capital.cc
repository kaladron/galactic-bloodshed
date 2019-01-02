// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* capital.c -- designate a capital */

#include "capital.h"

#include <cstdio>
#include <cstdlib>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "fire.h"
#include "getplace.h"
#include "races.h"
#include "ships.h"
#include "shlmisc.h"
#include "vars.h"

void capital(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  int APcount = 50;
  int snum;
  Ship *s;
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

  if (shipno <= 0) {
    g.out << "Change the capital to be what ship?\n";
    return;
  }

  auto stat = getship(&s, shipno);

  if (argv.size() == 2) {
    snum = s->storbits;
    if (!stat || testship(Playernum, Governor, s)) {
      g.out << "You can't do that!\n";
      free(s);
      return;
    }
    if (!landed(s)) {
      g.out << "Try landing this ship first!\n";
      free(s);
      return;
    }
    if (!enufAP(Playernum, Governor, Stars[snum]->AP[Playernum - 1], APcount)) {
      free(s);
      return;
    }
    if (s->type != ShipType::OTYPE_GOV) {
      sprintf(buf, "That ship is not a %s.\n", Shipnames[ShipType::OTYPE_GOV]);
      notify(Playernum, Governor, buf);
      free(s);
      return;
    }
    deductAPs(Playernum, Governor, APcount, snum, 0);
    Race->Gov_ship = shipno;
    putrace(Race);
  }

  sprintf(buf, "Efficiency of governmental center: %.0f%%.\n",
          ((double)s->popn / (double)Max_crew(s)) * (100 - (double)s->damage));
  notify(Playernum, Governor, buf);
  free(s);
}
