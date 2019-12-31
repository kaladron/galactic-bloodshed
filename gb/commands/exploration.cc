// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/commands/exploration.h"

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files_shl.h"
#include "gb/map.h"
#include "gb/max.h"
#include "gb/place.h"
#include "gb/power.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/tech.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

void exploration(const command_t &argv, GameObj &g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  int starq;
  int j;
  racetype *Race;

  starq = -1;

  if (argv.size() == 2) {
    Place where{g, argv[1]};
    if (where.err) {
      sprintf(buf, "explore: bad scope.\n");
      notify(Playernum, Governor, buf);
      return;
    }
    if (where.level == ScopeLevel::LEVEL_SHIP ||
        where.level == ScopeLevel::LEVEL_UNIV) {
      sprintf(buf, "Bad scope '%s'.\n", argv[1].c_str());
      notify(Playernum, Governor, buf);
      return;
    }
    starq = where.snum;
  }

  Race = races[Playernum - 1];

  getsdata(&Sdata);
  sprintf(buf, "         ========== Exploration Report ==========\n");
  notify(Playernum, Governor, buf);
  sprintf(buf, " Global action points : [%2d]\n", Sdata.AP[Playernum - 1]);
  notify(Playernum, Governor, buf);
  sprintf(
      buf,
      " Star  (stability)[AP]   #  Planet [Attributes] Type (Compatibility)\n");
  notify(Playernum, Governor, buf);
  for (starnum_t star = 0; star < Sdata.numstars; star++)
    if ((starq == -1) || (starq == star)) {
      getstar(&(Stars[star]), star);
      if (isset(Stars[star]->explored, Playernum))
        for (planetnum_t i = 0; i < Stars[star]->numplanets; i++) {
          const auto pl = getplanet(star, i);
          if (i == 0) {
            if (Race->tech >= TECH_SEE_STABILITY) {
              sprintf(buf, "\n%13s (%2d)[%2d]\n", Stars[star]->name,
                      Stars[star]->stability, Stars[star]->AP[Playernum - 1]);
              notify(Playernum, Governor, buf);
            } else {
              sprintf(buf, "\n%13s (/?/?)[%2d]\n", Stars[star]->name,
                      Stars[star]->AP[Playernum - 1]);
              notify(Playernum, Governor, buf);
            }
          }

          sprintf(buf, "\t\t      ");
          notify(Playernum, Governor, buf);

          sprintf(buf, "  #%d. %-15s [ ", i + 1, Stars[star]->pnames[i]);
          notify(Playernum, Governor, buf);
          if (pl.info[Playernum - 1].explored) {
            sprintf(buf, "Ex ");
            notify(Playernum, Governor, buf);
            if (pl.info[Playernum - 1].autorep) {
              sprintf(buf, "Rep ");
              notify(Playernum, Governor, buf);
            }
            if (pl.info[Playernum - 1].numsectsowned) {
              sprintf(buf, "Inhab ");
              notify(Playernum, Governor, buf);
            }
            if (pl.slaved_to) {
              sprintf(buf, "SLAVED ");
              notify(Playernum, Governor, buf);
            }
            for (j = 1; j <= Num_races; j++)
              if (j != Playernum && pl.info[j - 1].numsectsowned) {
                sprintf(buf, "%d ", j);
                notify(Playernum, Governor, buf);
              }
            if (pl.conditions[TOXIC] > 70) {
              sprintf(buf, "TOXIC ");
              notify(Playernum, Governor, buf);
            }
            sprintf(buf, "] %s %2.0f%%\n", Planet_types[pl.type],
                    pl.compatibility(*Race));
            notify(Playernum, Governor, buf);
          } else {
            sprintf(buf, "No Data ]\n");
            notify(Playernum, Governor, buf);
          }
        }
    }
}
