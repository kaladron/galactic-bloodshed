// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* scrap.c -- turn a ship to junk */

#include "scrap.h"

#include <stdio.h>
#include <stdlib.h>

#include "GB_server.h"
#include "buffers.h"
#include "files_shl.h"
#include "fire.h"
#include "land.h"
#include "load.h"
#include "races.h"
#include "ships.h"
#include "shlmisc.h"
#include "vars.h"

void scrap(int Playernum, int Governor, int APcount) {
  planettype *planet;
  sector sect;
  shiptype *s, *s2;
  shipnum_t shipno, nextshipno;
  int scrapval = 0, destval = 0, crewval = 0, xtalval = 0, troopval = 0;
  double fuelval = 0.0;
  racetype *Race;

  if (argn < 2) {
    notify(Playernum, Governor, "Scrap what?\n");
    return;
  }

  nextshipno = start_shiplist(Playernum, Governor, args[1]);

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(Playernum, args[1], s, &nextshipno) &&
        authorized(Governor, s)) {
      if (s->max_crew && !s->popn) {
        notify(Playernum, Governor, "Can't scrap that ship - no crew.\n");
        free(s);
        continue;
      }
      if (s->whatorbits == LEVEL_UNIV) {
        continue;
      } else if (!enufAP(Playernum, Governor,
                         Stars[s->storbits]->AP[Playernum - 1], APcount)) {
        free(s);
        continue;
      }
      if (s->whatorbits == LEVEL_PLAN && s->type == OTYPE_TOXWC) {
        sprintf(buf,
                "WARNING: This will release %d toxin points back into the "
                "atmosphere!!\n",
                s->special.waste.toxic);
        notify(Playernum, Governor, buf);
      }
      if (!s->docked) {
        sprintf(buf,
                "%s is not landed or docked.\nNo resources can be reclaimed.\n",
                Ship(*s).c_str());
        notify(Playernum, Governor, buf);
      }
      if (s->whatorbits == LEVEL_PLAN) {
        /* wc's release poison */
        getplanet(&planet, (int)s->storbits, (int)s->pnumorbits);
        if (landed(s)) sect = getsector(*planet, s->land_x, s->land_y);
      }
      if (docked(s)) {
        if (!getship(&s2, (int)(s->destshipno))) {
          free(s);
          continue;
        }
        if (!(s2->docked && s2->destshipno == s->number) &&
            !s->whatorbits == LEVEL_SHIP) {
          sprintf(buf, "Warning, other ship not docked..\n");
          notify(Playernum, Governor, buf);
          free(s);
          free(s2);
          continue;
        }
      }

      scrapval = Cost(s) / 2 + s->resource;

      if (s->docked) {
        sprintf(buf, "%s: original cost: %ld\n", Ship(*s).c_str(), Cost(s));
        notify(Playernum, Governor, buf);
        sprintf(buf, "         scrap value%s: %d rp's.\n",
                s->resource ? "(with stockpile) " : "", scrapval);
        notify(Playernum, Governor, buf);

        if (s->whatdest == LEVEL_SHIP &&
            s2->resource + scrapval > Max_resource(s2) &&
            s2->type != STYPE_SHUTTLE) {
          scrapval = Max_resource(s2) - s2->resource;
          sprintf(buf, "(There is only room for %d resources.)\n", scrapval);
          notify(Playernum, Governor, buf);
        }
        if (s->fuel) {
          sprintf(buf, "Fuel recovery: %.0f.\n", s->fuel);
          notify(Playernum, Governor, buf);
          fuelval = s->fuel;
          if (s->whatdest == LEVEL_SHIP && s2->fuel + fuelval > Max_fuel(s2)) {
            fuelval = Max_fuel(s2) - s2->fuel;
            sprintf(buf, "(There is only room for %.2f fuel.)\n", fuelval);
            notify(Playernum, Governor, buf);
          }
        } else
          fuelval = 0.0;

        if (s->destruct) {
          sprintf(buf, "Weapons recovery: %d.\n", s->destruct);
          notify(Playernum, Governor, buf);
          destval = s->destruct;
          if (s->whatdest == LEVEL_SHIP &&
              s2->destruct + destval > Max_destruct(s2)) {
            destval = Max_destruct(s2) - s2->destruct;
            sprintf(buf, "(There is only room for %d destruct.)\n", destval);
            notify(Playernum, Governor, buf);
          }
        } else
          destval = 0;

        if (s->popn + s->troops) {
          if (s->whatdest == LEVEL_PLAN && sect.owner > 0 &&
              sect.owner != Playernum) {
            sprintf(buf,
                    "You don't own this sector; no crew can be recovered.\n");
            notify(Playernum, Governor, buf);
          } else {
            sprintf(buf, "Population/Troops recovery: %lu/%lu.\n", s->popn,
                    s->troops);
            notify(Playernum, Governor, buf);
            troopval = s->troops;
            if (s->whatdest == LEVEL_SHIP &&
                s2->troops + troopval > Max_mil(s2)) {
              troopval = Max_mil(s2) - s2->troops;
              sprintf(buf, "(There is only room for %d troops.)\n", troopval);
              notify(Playernum, Governor, buf);
            }
            crewval = s->popn;
            if (s->whatdest == LEVEL_SHIP &&
                s2->popn + crewval > Max_crew(s2)) {
              crewval = Max_crew(s2) - s2->popn;
              sprintf(buf, "(There is only room for %d crew.)\n", crewval);
              notify(Playernum, Governor, buf);
            }
          }
        } else {
          crewval = 0;
          troopval = 0;
        }

        if (s->crystals + s->mounted) {
          if (s->whatdest == LEVEL_PLAN && sect.owner > 0 &&
              sect.owner != Playernum) {
            sprintf(
                buf,
                "You don't own this sector; no crystals can be recovered.\n");
            notify(Playernum, Governor, buf);
          } else {
            xtalval = s->crystals + s->mounted;
            if (s->whatdest == LEVEL_SHIP &&
                s2->crystals + xtalval > Max_crystals(s2)) {
              xtalval = Max_crystals(s2) - s2->crystals;
              sprintf(buf, "(There is only room for %d crystals.)\n", xtalval);
              notify(Playernum, Governor, buf);
            }
            sprintf(buf, "Crystal recovery: %d.\n", xtalval);
            notify(Playernum, Governor, buf);
          }
        } else
          xtalval = 0;
      }

      /* more adjustments needed here for hanger. Maarten */
      if (s->whatorbits == LEVEL_SHIP) s2->hanger -= s->size;

      if (s->whatorbits == LEVEL_UNIV)
        deductAPs(Playernum, Governor, APcount, 0, 1);
      else
        deductAPs(Playernum, Governor, APcount, (int)s->storbits, 0);

      Race = races[Playernum - 1];
      kill_ship(Playernum, s);
      putship(s);
      if (docked(s)) {
#ifdef NEVER
        fuelval = MIN(fuelval, 1. * Max_fuel(s2) - s2->fuel);
        destval = MIN(destval, Max_destruct(s2) - s2->destruct);
        if (s2->type != STYPE_SHUTTLE) /* Leave scrapval alone for shuttles */
          scrapval = MIN(scrapval, Max_resource(s2) - s2->resource);
        troopval = MIN(troopval, Max_crew(s2) - s2->troops);
        crewval = MIN(crewval, Max_crew(s2) - s2->popn);
        xtalval = MIN(xtalval, Max_crystals(s2) - s2->crystals);
#endif
        s2->crystals += xtalval;
        rcv_fuel(s2, (double)fuelval);
        rcv_destruct(s2, destval);
        rcv_resource(s2, scrapval);
        rcv_troops(s2, troopval, Race->mass);
        rcv_popn(s2, crewval, Race->mass);
        /* check for docking status in case scrapped ship is landed. Maarten */
        if (!(s->whatorbits == LEVEL_SHIP)) {
          s2->docked = 0; /* undock the surviving ship */
          s2->whatdest = LEVEL_UNIV;
          s2->destshipno = 0;
        }
        putship(s2);
        free(s2);
      }

      if (s->whatorbits == LEVEL_PLAN) {
        free(planet); /* This has already been allocated */
        getplanet(&planet, (int)s->storbits, (int)s->pnumorbits);
        if (landed(s)) {
          if (sect.owner == Playernum) {
            sect.popn += troopval;
            sect.popn += crewval;
          } else if (sect.owner == 0) {
            sect.owner = Playernum;
            sect.popn += crewval;
            sect.troops += troopval;
            planet->info[Playernum - 1].numsectsowned++;
            planet->info[Playernum - 1].popn += crewval;
            planet->info[Playernum - 1].popn += troopval;
            sprintf(buf, "Sector %d,%d Colonized.\n", s->land_x, s->land_y);
            notify(Playernum, Governor, buf);
          }
          planet->info[Playernum - 1].resource += scrapval;
          planet->popn += crewval;
          planet->info[Playernum - 1].destruct += destval;
          planet->info[Playernum - 1].fuel += (int)fuelval;
          planet->info[Playernum - 1].crystals += (int)xtalval;
          putsector(sect, *planet, s->land_x, s->land_y);
        }
        putplanet(planet, Stars[s->storbits], (int)s->pnumorbits);
        free(planet);
      }
      if (landed(s)) {
        sprintf(buf, "\nScrapped.\n");
        notify(Playernum, Governor, buf);
      } else {
        sprintf(buf, "\nDestroyed.\n");
        notify(Playernum, Governor, buf);
      }
      free(s);
    } else
      free(s);
}
