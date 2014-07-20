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

static const char *allied(racetype *, int, int, int);

void relation(int Playernum, int Governor, int APcount) {
  int p, q;
  racetype *r, *Race;

  unsigned long numraces = Num_races;

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
  for (p = 1; p <= numraces; p++)
    if (p != Race->Playernum) {
      r = races[p - 1];
      sprintf(buf, "%2d %s (%3d%%) %20.20s : %10s   %10s\n", p,
              ((Race->God || (Race->translate[p - 1] > 30)) && r->Metamorph &&
               (Playernum == q))
                  ? "Morph"
                  : "     ",
              Race->translate[p - 1], r->name,
              allied(Race, p, 100, (int)Race->God),
              allied(r, q, (int)Race->translate[p - 1], (int)Race->God));
      notify(Playernum, Governor, buf);
    }
}

static const char *allied(racetype *r, int p, int q, int God) {
  if (isset(r->atwar, p))
    return "WAR";
  else if (isset(r->allied, p))
    return "ALLIED";
  else
    return "neutral";
}
