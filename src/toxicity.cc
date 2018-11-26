// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* toxicty.c -- change threshold in toxicity to build a wc. */

#include "toxicity.h"

#include <stdio.h>
#include <stdlib.h>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "shlmisc.h"
#include "vars.h"

void toxicity(int Playernum, int Governor, int APcount) {
  int thresh;

  sscanf(args[1], "%d", &thresh);

  if (thresh > 100 || thresh < 0) {
    sprintf(buf, "Illegal value.\n");
    notify(Playernum, Governor, buf);
    return;
  }

  if (Dir[Playernum - 1][Governor].level != ScopeLevel::LEVEL_PLAN) {
    sprintf(buf, "scope must be a planet.\n");
    notify(Playernum, Governor, buf);
    return;
  }
  if (!enufAP(Playernum, Governor,
              Stars[Dir[Playernum - 1][Governor].snum]->AP[Playernum - 1],
              APcount)) {
    return;
  }

  auto p = getplanet(Dir[Playernum - 1][Governor].snum,
                     Dir[Playernum - 1][Governor].pnum);
  p.info[Playernum - 1].tox_thresh = thresh;
  putplanet(p, Stars[Dir[Playernum - 1][Governor].snum],
            Dir[Playernum - 1][Governor].pnum);
  deductAPs(Playernum, Governor, APcount, Dir[Playernum - 1][Governor].snum, 0);

  sprintf(buf, " New threshold is: %u\n", p.info[Playernum - 1].tox_thresh);
  notify(Playernum, Governor, buf);
}
