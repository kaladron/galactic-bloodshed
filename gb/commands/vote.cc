// Copyright 2020 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std.compat;

#include "gb/GB_server.h"
#include "gb/files_shl.h"
#include "gb/races.h"

module commands;

namespace {
void show_votes(GameObj& g) {
  int nvotes = 0;
  int nays = 0;
  int yays = 0;

  for (const auto& race : races) {
    if (race.God || race.Guest) continue;
    nvotes++;
    if (race.votes) {
      yays++;
      if (g.god) g.out << std::format("  {0} voted go.\n", race.name);
    } else {
      nays++;
      if (g.god) g.out << std::format("  {0} voted wait.\n", race.name);
    }
  }
  g.out << std::format("  Total votes = {0}, Go = {1}, Wait = {2}.\n", nvotes,
                       yays, nays);
}
}  // namespace

void vote(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;

  auto& race = races[Playernum - 1];

  if (g.god) {
    g.out << "Your vote doesn't count, however, here is the count.\n";
    show_votes(g);
    return;
  }
  if (race.Guest) {
    g.out << "You are not allowed to vote, but, here is the count.\n";
    show_votes(g);
    return;
  }

  if (argv.size() <= 2) {
    g.out << std::format("Your vote on updates is {0}\n",
                         race.votes ? "go" : "wait");
    show_votes(g);
    return;
  }

  bool check = false;
  if (argv[1] == "update") {
    if (argv[2] == "go") {
      race.votes = true;
      check = true;
    } else if (argv[2] == "wait")
      race.votes = false;
    else {
      g.out << std::format("No such update choice '{0}'\n", argv[2].c_str());
      return;
    }
  } else {
    g.out << std::format("No such vote '{0}'\n", argv[1].c_str());
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
}
