// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/commands/victory.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/races.h"
#include "gb/vars.h"
#include "gb/victory.h"

void victory(const command_t &argv, GameObj &g) {
  int count = (argv.size() > 1) ? std::stoi(argv[1]) : Num_races;
  if (count > Num_races) count = Num_races;

  auto vic = create_victory_list();

  g.out << "----==== PLAYER RANKINGS ====----\n";
  sprintf(buf, "%-4.4s %-15.15s %8s\n", "No.", "Name", (g.god ? "Score" : ""));
  notify(g.player, g.governor, buf);
  for (int i = 0; i < count; i++) {
    if (g.god)
      sprintf(buf, "%2d %c [%2d] %-15.15s %5ld  %6.2f %3d %s %s\n", i + 1,
              vic[i].Thing ? 'M' : ' ', vic[i].racenum, vic[i].name.c_str(),
              vic[i].rawscore, vic[i].tech, vic[i].IQ,
              races[vic[i].racenum - 1]->password,
              races[vic[i].racenum - 1]->governor[0].password);
    else
      sprintf(buf, "%2d   [%2d] %-15.15s\n", i + 1, vic[i].racenum,
              vic[i].name.c_str());
    notify(g.player, g.governor, buf);
  }
}
