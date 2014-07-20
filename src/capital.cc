// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* capital.c -- designate a capital */

#include "capital.h"

#include <stdio.h>
#include <stdlib.h>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "fire.h"
#include "getplace.h"
#include "races.h"
#include "ships.h"
#include "shlmisc.h"
#include "vars.h"

void capital(int Playernum, int Governor, int APcount) {
  int stat, snum;
  shiptype *s;
  racetype *Race;

  Race = races[Playernum - 1];
  if (Governor) {
    (void)notify(Playernum, Governor,
                 "Only the leader may designate the capital.\n");
    return;
  }

  shipnum_t shipno;
  if (argn != 2)
    shipno = Race->Gov_ship;
  else
    shipno = strtoul(args[1] + (args[1][0] == '#'), NULL, 10);

  if (shipno <= 0) {
    (void)notify(Playernum, Governor, "Change the capital to be what ship?\n");
    return;
  }

  stat = getship(&s, shipno);

  if (argn == 2) {
    snum = s->storbits;
    if (!stat || testship(Playernum, Governor, s)) {
      (void)notify(Playernum, Governor, "You can't do that!\n");
      free(s);
      return;
    }
    if (!landed(s)) {
      (void)notify(Playernum, Governor, "Try landing this ship first!\n");
      free(s);
      return;
    }
    if (!enufAP(Playernum, Governor, Stars[snum]->AP[Playernum - 1], APcount)) {
      free(s);
      return;
    }
    if (s->type != OTYPE_GOV) {
      sprintf(buf, "That ship is not a %s.\n", Shipnames[OTYPE_GOV]);
      (void)notify(Playernum, Governor, buf);
      free(s);
      return;
    }
    deductAPs(Playernum, Governor, APcount, snum, 0);
    Race->Gov_ship = shipno;
    putrace(Race);
  }

  sprintf(buf, "Efficiency of governmental center: %.0f%%.\n",
          ((double)s->popn / (double)Max_crew(s)) * (100 - (double)s->damage));
  (void)notify(Playernum, Governor, buf);
  free(s);
}
