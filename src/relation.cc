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

static const char *allied(const race *const, const player_t);

void relation(const command_t &argv, const player_t Playernum,
              const governor_t Governor, const int APcount) {

  player_t q;
  if (argv.size() == 1) {
    q = Playernum;
  } else {
    if (!(q = GetPlayer(argv[1].c_str()))) {
      notify(Playernum, Governor, "No such player.\n");
      return;
    }
  }

  auto Race = races[q - 1];

  sprintf(buf, "\n              Racial Relations Report for %s\n\n",
          Race->name);
  notify(Playernum, Governor, buf);
  sprintf(buf,
          " #       know             Race name       Yours        Theirs\n");
  notify(Playernum, Governor, buf);
  sprintf(buf,
          " -       ----             ---------       -----        ------\n");
  notify(Playernum, Governor, buf);
  for (player_t p = 1; p <= Num_races; p++)
    if (p != Race->Playernum) {
      auto r = races[p - 1];
      sprintf(buf, "%2hhu %s (%3d%%) %20.20s : %10s   %10s\n", p,
              ((Race->God || (Race->translate[p - 1] > 30)) && r->Metamorph &&
               (Playernum == q))
                  ? "Morph"
                  : "     ",
              Race->translate[p - 1], r->name, allied(Race, p), allied(r, q));
      notify(Playernum, Governor, buf);
    }
}

static const char *allied(const race *const r, const player_t p) {
  if (isset(r->atwar, p))
    return "WAR";
  else if (isset(r->allied, p))
    return "ALLIED";
  else
    return "neutral";
}
