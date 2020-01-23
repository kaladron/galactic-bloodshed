// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* declare.c -- declare alliance, neutrality, war, the basic thing. */

import gblib;
import std;

#include "gb/commands/vote.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/config.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/races.h"
#include "gb/shlmisc.h"
#include "gb/tele.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

namespace {
void show_votes(player_t Playernum, governor_t Governor) {
  int nvotes = 0;
  int nays = 0;
  int yays = 0;

  for (const auto& race : races) {
    if (race.God || race.Guest) continue;
    nvotes++;
    if (race.votes) {
      yays++;
      sprintf(buf, "  %s voted go.\n", race.name);
    } else {
      nays++;
      sprintf(buf, "  %s voted wait.\n", race.name);
    }
    if (races[Playernum - 1].God) notify(Playernum, Governor, buf);
  }
  sprintf(buf, "  Total votes = %d, Go = %d, Wait = %d.\n", nvotes, yays, nays);
  notify(Playernum, Governor, buf);
}
}  // namespace

void vote(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;

  auto& race = races[Playernum - 1];

  if (race.God) {
    g.out << "Your vote doesn't count, however, here is the count.\n";
    show_votes(Playernum, Governor);
    return;
  }
  if (race.Guest) {
    g.out << "You are not allowed to vote, but, here is the count.\n";
    notify(Playernum, Governor, buf);
    show_votes(Playernum, Governor);
    return;
  }

  if (argv.size() > 2) {
    bool check = false;
    if (argv[1] == "update") {
      if (argv[2] == "go") {
        race.votes = true;
        check = true;
      } else if (argv[2] == "wait")
        race.votes = false;
      else {
        sprintf(buf, "No such update choice '%s'\n", argv[2].c_str());
        notify(Playernum, Governor, buf);
        return;
      }
    } else {
      sprintf(buf, "No such vote '%s'\n", argv[1].c_str());
      notify(Playernum, Governor, buf);
      return;
    }
    putrace(race);

    if (check) {
      /* Ok...someone voted yes.  Tally them all up and see if */
      /* we should do something. */
      int nays = 0;
      int yays = 0;
      int nvotes = 0;
      for (const auto& r : races) {
        if (r.God || r.Guest) continue;
        nvotes++;
        if (r.votes)
          yays++;
        else
          nays++;
      }
      /* Is Update/Movement vote unanimous now? */
      if (nvotes > 0 && nvotes == yays && nays == 0) {
        /* Do it... */
        do_next_thing(g.db);
      }
    }
  } else {
    sprintf(buf, "Your vote on updates is %s\n", race.votes ? "go" : "wait");
    notify(Playernum, Governor, buf);
    show_votes(Playernum, Governor);
  }
}
