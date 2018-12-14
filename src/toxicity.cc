// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* toxicty.c -- change threshold in toxicity to build a wc. */

#include "toxicity.h"

#include <cstdio>
#include <cstdlib>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "shlmisc.h"
#include "vars.h"

void toxicity(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int thresh;
  int APcount = 1;

  if (argv.size() != 2) {
    std::string response = "Provide exactly one value between 0 and 100.\n";
    notify(Playernum, Governor, response);
    return;
  }

  sscanf(argv[1].c_str(), "%d", &thresh);

  if (thresh > 100 || thresh < 0) {
    std::string response = "Illegal value.\n";
    notify(Playernum, Governor, response);
    return;
  }

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    std::string response = "scope must be a planet.\n";
    notify(Playernum, Governor, response);
    return;
  }
  if (!enufAP(Playernum, Governor, Stars[g.snum]->AP[Playernum - 1], APcount)) {
    return;
  }

  auto p = getplanet(g.snum, g.pnum);
  p.info[Playernum - 1].tox_thresh = thresh;
  putplanet(p, Stars[g.snum], g.pnum);
  deductAPs(Playernum, Governor, APcount, g.snum, 0);

  sprintf(buf, " New threshold is: %u\n", p.info[Playernum - 1].tox_thresh);
  notify(Playernum, Governor, buf);
}
