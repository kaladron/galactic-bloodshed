// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* relation.c -- state relations among players */

module;

import gblib;
import std.compat;

#include "gb/buffers.h"

module commands;

static auto allied(const Race &r, const player_t p) {
  if (isset(r.atwar, p)) return "WAR";
  if (isset(r.allied, p)) return "ALLIED";
  return "neutral";
}

namespace GB::commands {
void relation(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  player_t q;
  if (argv.size() == 1) {
    q = Playernum;
  } else {
    if (!(q = get_player(argv[1]))) {
      g.out << "No such player.\n";
      return;
    }
  }

  auto &race = races[q - 1];

  sprintf(buf, "\n              Racial Relations Report for %s\n\n", race.name);
  notify(Playernum, Governor, buf);
  g.out << " #       know             Race name       Yours        Theirs\n";
  g.out << " -       ----             ---------       -----        ------\n";
  for (auto r : races) {
    if (r.Playernum == race.Playernum) continue;
    sprintf(buf, "%2u %s (%3d%%) %20.20s : %10s   %10s\n", r.Playernum,
            ((race.God || (race.translate[r.Playernum - 1] > 30)) &&
             r.Metamorph && (Playernum == q))
                ? "Morph"
                : "     ",
            race.translate[r.Playernum - 1], r.name, allied(race, r.Playernum),
            allied(r, q));
    notify(Playernum, Governor, buf);
  }
}
}  // namespace GB::commands
