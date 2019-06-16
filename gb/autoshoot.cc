// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* autoshoot() -- shoot <-> retaliate routine
 * Bombard() -- ship bombards planet
 */

#include "gb/autoshoot.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/files.h"
#include "gb/files_shl.h"
#include "gb/max.h"
#include "gb/perm.h"
#include "gb/races.h"
#include "gb/ships.h"
#include "gb/shootblast.h"
#include "gb/tele.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

/* ship #shipno bombards planet, then alert whom it may concern.
 */
int Bombard(Ship *ship, Planet *planet, Race *r) {
  int x;
  int y;
  int x2 = -1;
  int y2;
  int oldown;
  int numdest = 0;
  int found = 0;

  /* for telegramming */
  bzero((char *)Nuked, sizeof(Nuked));

  /* check to see if PDNs are present */
  Shiplist shiplist(planet->ships);
  for (auto s : shiplist) {
    if (s.alive && s.type == ShipType::OTYPE_PLANDEF &&
        s.owner != ship->owner) {
      sprintf(buf, "Bombardment of %s cancelled, PDNs are present.\n",
              prin_ship_orbits(ship));
      warn(ship->owner, ship->governor, buf);
      return 0;
    }
  }

  auto smap = getsmap(*planet);

  /* look for someone to bombard-check for war */
  (void)Getxysect(*planet, nullptr, nullptr, 1); /* reset */
  while (!found && Getxysect(*planet, &x, &y, 0)) {
    if (smap.get(x, y).owner && smap.get(x, y).owner != ship->owner &&
        (smap.get(x, y).condition != SectorType::SEC_WASTED)) {
      if (isset(r->atwar, smap.get(x, y).owner) ||
          (ship->type == ShipType::OTYPE_BERS &&
           smap.get(x, y).owner == ship->special.mind.target))
        found = 1;
      else {
        x2 = x;
        y2 = y;
      }
    }
  }
  if (x2 != -1) {
    x = x2; /* no one we're at war with; bomb someone else. */
    y = y2;
    found = 1;
  }

  if (found) {
    int str;
    str = MIN(Shipdata[ship->type][ABIL_GUNS] * (100 - ship->damage) / 100.,
              ship->destruct);
    /* save owner of destroyed sector */
    if (str) {
      bzero(Nuked, sizeof(Nuked));
      oldown = smap.get(x, y).owner;
      ship->destruct -= str;
      ship->mass -= str * MASS_DESTRUCT;

      numdest = shoot_ship_to_planet(ship, planet, str, x, y, smap, 0, 0,
                                     long_buf, short_buf);
      /* (0=dont get smap) */
      if (numdest < 0) numdest = 0;

      /* tell the bombarding player about it.. */
      sprintf(telegram_buf, "REPORT from ship #%lu\n\n", ship->number);
      strcat(telegram_buf, short_buf);
      sprintf(buf, "sector %d,%d (owner %d).  %d sectors destroyed.\n", x, y,
              oldown, numdest);
      strcat(telegram_buf, buf);
      notify(ship->owner, ship->governor, telegram_buf);

      /* notify other player. */
      sprintf(telegram_buf, "ALERT from planet /%s/%s\n",
              Stars[ship->storbits]->name,
              Stars[ship->storbits]->pnames[ship->pnumorbits]);
      sprintf(buf, "%c%lu%s bombarded sector %d,%d; %d sectors destroyed.\n",
              Shipltrs[ship->type], ship->number, ship->name, x, y, numdest);
      strcat(telegram_buf, buf);
      sprintf(buf, "%c%lu %s [%d] bombards %s/%s\n", Shipltrs[ship->type],
              ship->number, ship->name, ship->owner,
              Stars[ship->storbits]->name,
              Stars[ship->storbits]->pnames[ship->pnumorbits]);
      for (player_t i = 1; i <= Num_races; i++)
        if (Nuked[i - 1] && i != ship->owner)
          warn(i, Stars[ship->storbits]->governor[i - 1], telegram_buf);
      post(buf, COMBAT);

      /* enemy planet retaliates along with defending forces */
    } else {
      /* no weapons! */
      if (!ship->notified) {
        ship->notified = 1;
        sprintf(telegram_buf,
                "Bulletin\n\n %c%lu %s has no weapons to bombard with.\n",
                Shipltrs[ship->type], ship->number, ship->name);
        warn(ship->owner, ship->governor, telegram_buf);
      }
    }

    putsmap(smap, *planet);

  } else {
    /* there were no sectors worth bombing. */
    if (!ship->notified) {
      ship->notified = 1;
      sprintf(telegram_buf, "Report from %c%lu %s\n\n", Shipltrs[ship->type],
              ship->number, ship->name);
      sprintf(buf, "Planet /%s/%s has been saturation bombed.\n",
              Stars[ship->storbits]->name,
              Stars[ship->storbits]->pnames[ship->pnumorbits]);
      strcat(telegram_buf, buf);
      notify(ship->owner, ship->governor, telegram_buf);
    }
  }
  return numdest;
}
