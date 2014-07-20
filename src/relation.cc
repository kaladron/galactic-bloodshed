// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* relation.c -- state relations among players */

#include "relation.h"

#include <stdio.h>

#include "GB_server.h"
#include "buffers.h"
#include "races.h"
#include "shlmisc.h"
#include "vars.h"

static const char *allied(racetype *, int);

void relation(player_t Playernum, governor_t Governor, int APcount) {
  int q;
  racetype *Race;

  if (argn == 1) {
    q = Playernum;
  } else {
    if (!(q = GetPlayer(args[1]))) {
      notify(Playernum, Governor, "No such player.\n");
      return;
    }
  }

  Race = races[q - 1];

  sprintf(buf, "\n              Racial Relations Report for %s\n\n",
          Race->name);
  notify(Playernum, Governor, buf);
  sprintf(buf,
          " #       know             Race name       Yours        Theirs\n");
  notify(Playernum, Governor, buf);
  sprintf(buf,
          " -       ----             ---------       -----        ------\n");
  notify(Playernum, Governor, buf);
  for (unsigned long p = 1; p <= Num_races; p++)
    if (p != Race->Playernum) {
      racetype *r = races[p - 1];
      sprintf(buf, "%2lu %s (%3d%%) %20.20s : %10s   %10s\n", p,
              ((Race->God || (Race->translate[p - 1] > 30)) && r->Metamorph &&
               (Playernum == q))
                  ? "Morph"
                  : "     ",
              Race->translate[p - 1], r->name,
              allied(Race, p),
              allied(r, q));
      notify(Playernum, Governor, buf);
    }
}

static const char *allied(racetype *r, int p) {
  if (isset(r->atwar, p))
    return "WAR";
  else if (isset(r->allied, p))
    return "ALLIED";
  else
    return "neutral";
}
