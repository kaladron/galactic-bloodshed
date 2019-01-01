// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* scrap.c -- turn a ship to junk */

#include "scrap.h"

#include <cstdio>
#include <cstdlib>

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

void scrap(const command_t &argv, GameObj &g) {
  int APcount = 1;
  sector sect;
  Ship *s, *s2;
  shipnum_t shipno, nextshipno;
  int scrapval = 0, destval = 0, crewval = 0, xtalval = 0, troopval = 0;
  double fuelval = 0.0;
  racetype *Race;

  if (argv.size() < 2) {
    g.out << "Scrap what?\n";
    return;
  }

  nextshipno = start_shiplist(g, argv[1]);

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(g.player, argv[1].c_str(), s, &nextshipno) &&
        authorized(g.governor, s)) {
      if (s->max_crew && !s->popn) {
        notify(g.player, g.governor, "Can't scrap that ship - no crew.\n");
        free(s);
        continue;
      }
      if (s->whatorbits == ScopeLevel::LEVEL_UNIV) {
        continue;
      }
      if (!enufAP(g.player, g.governor, Stars[s->storbits]->AP[g.player - 1],
                  APcount)) {
        free(s);
        continue;
      }
      if (s->whatorbits == ScopeLevel::LEVEL_PLAN &&
          s->type == ShipType::OTYPE_TOXWC) {
        sprintf(buf,
                "WARNING: This will release %d toxin points back into the "
                "atmosphere!!\n",
                s->special.waste.toxic);
        notify(g.player, g.governor, buf);
      }
      if (!s->docked) {
        sprintf(buf,
                "%s is not landed or docked.\nNo resources can be reclaimed.\n",
                ship_to_string(*s).c_str());
        notify(g.player, g.governor, buf);
      }
      if (s->whatorbits == ScopeLevel::LEVEL_PLAN) {
        /* wc's release poison */
        const auto &planet = getplanet((int)s->storbits, (int)s->pnumorbits);
        if (landed(s)) sect = getsector(planet, s->land_x, s->land_y);
      }
      if (docked(s)) {
        if (!getship(&s2, (int)(s->destshipno))) {
          free(s);
          continue;
        }
        // TODO(jeffbailey): Changed from !s->whatorbits, which didn't make any
        // sense.
        if (!(s2->docked && s2->destshipno == s->number) &&
            s->whatorbits != ScopeLevel::LEVEL_SHIP) {
          g.out << "Warning, other ship not docked..\n";
          free(s);
          free(s2);
          continue;
        }
      }

      scrapval = Cost(s) / 2 + s->resource;

      if (s->docked) {
        sprintf(buf, "%s: original cost: %ld\n", ship_to_string(*s).c_str(),
                Cost(s));
        notify(g.player, g.governor, buf);
        sprintf(buf, "         scrap value%s: %d rp's.\n",
                s->resource ? "(with stockpile) " : "", scrapval);
        notify(g.player, g.governor, buf);

        if (s->whatdest == ScopeLevel::LEVEL_SHIP &&
            s2->resource + scrapval > Max_resource(s2) &&
            s2->type != ShipType::STYPE_SHUTTLE) {
          scrapval = Max_resource(s2) - s2->resource;
          sprintf(buf, "(There is only room for %d resources.)\n", scrapval);
          notify(g.player, g.governor, buf);
        }
        if (s->fuel) {
          sprintf(buf, "Fuel recovery: %.0f.\n", s->fuel);
          notify(g.player, g.governor, buf);
          fuelval = s->fuel;
          if (s->whatdest == ScopeLevel::LEVEL_SHIP &&
              s2->fuel + fuelval > Max_fuel(s2)) {
            fuelval = Max_fuel(s2) - s2->fuel;
            sprintf(buf, "(There is only room for %.2f fuel.)\n", fuelval);
            notify(g.player, g.governor, buf);
          }
        } else
          fuelval = 0.0;

        if (s->destruct) {
          sprintf(buf, "Weapons recovery: %d.\n", s->destruct);
          notify(g.player, g.governor, buf);
          destval = s->destruct;
          if (s->whatdest == ScopeLevel::LEVEL_SHIP &&
              s2->destruct + destval > Max_destruct(s2)) {
            destval = Max_destruct(s2) - s2->destruct;
            sprintf(buf, "(There is only room for %d destruct.)\n", destval);
            notify(g.player, g.governor, buf);
          }
        } else
          destval = 0;

        if (s->popn + s->troops) {
          if (s->whatdest == ScopeLevel::LEVEL_PLAN && sect.owner > 0 &&
              sect.owner != g.player) {
            g.out << "You don't own this sector; no crew can be recovered.\n";
          } else {
            sprintf(buf, "Population/Troops recovery: %lu/%lu.\n", s->popn,
                    s->troops);
            notify(g.player, g.governor, buf);
            troopval = s->troops;
            if (s->whatdest == ScopeLevel::LEVEL_SHIP &&
                s2->troops + troopval > Max_mil(s2)) {
              troopval = Max_mil(s2) - s2->troops;
              sprintf(buf, "(There is only room for %d troops.)\n", troopval);
              notify(g.player, g.governor, buf);
            }
            crewval = s->popn;
            if (s->whatdest == ScopeLevel::LEVEL_SHIP &&
                s2->popn + crewval > Max_crew(s2)) {
              crewval = Max_crew(s2) - s2->popn;
              sprintf(buf, "(There is only room for %d crew.)\n", crewval);
              notify(g.player, g.governor, buf);
            }
          }
        } else {
          crewval = 0;
          troopval = 0;
        }

        if (s->crystals + s->mounted) {
          if (s->whatdest == ScopeLevel::LEVEL_PLAN && sect.owner > 0 &&
              sect.owner != g.player) {
            g.out
                << "You don't own this sector; no crystals can be recovered.\n";
          } else {
            xtalval = s->crystals + s->mounted;
            if (s->whatdest == ScopeLevel::LEVEL_SHIP &&
                s2->crystals + xtalval > Max_crystals(s2)) {
              xtalval = Max_crystals(s2) - s2->crystals;
              sprintf(buf, "(There is only room for %d crystals.)\n", xtalval);
              notify(g.player, g.governor, buf);
            }
            sprintf(buf, "Crystal recovery: %d.\n", xtalval);
            notify(g.player, g.governor, buf);
          }
        } else
          xtalval = 0;
      }

      /* more adjustments needed here for hanger. Maarten */
      if (s->whatorbits == ScopeLevel::LEVEL_SHIP) s2->hanger -= s->size;

      if (s->whatorbits == ScopeLevel::LEVEL_UNIV)
        deductAPs(g.player, g.governor, APcount, 0, 1);
      else
        deductAPs(g.player, g.governor, APcount, s->storbits, 0);

      Race = races[g.player - 1];
      kill_ship(g.player, s);
      putship(s);
      if (docked(s)) {
#ifdef NEVER
        fuelval = MIN(fuelval, 1. * Max_fuel(s2) - s2->fuel);
        destval = MIN(destval, Max_destruct(s2) - s2->destruct);
        if (s2->type !=
            ShipType::STYPE_SHUTTLE) /* Leave scrapval alone for shuttles */
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
        if (!(s->whatorbits == ScopeLevel::LEVEL_SHIP)) {
          s2->docked = 0; /* undock the surviving ship */
          s2->whatdest = ScopeLevel::LEVEL_UNIV;
          s2->destshipno = 0;
        }
        putship(s2);
        free(s2);
      }

      if (s->whatorbits == ScopeLevel::LEVEL_PLAN) {
        auto planet = getplanet(s->storbits, s->pnumorbits);
        if (landed(s)) {
          if (sect.owner == g.player) {
            sect.popn += troopval;
            sect.popn += crewval;
          } else if (sect.owner == 0) {
            sect.owner = g.player;
            sect.popn += crewval;
            sect.troops += troopval;
            planet.info[g.player - 1].numsectsowned++;
            planet.info[g.player - 1].popn += crewval;
            planet.info[g.player - 1].popn += troopval;
            sprintf(buf, "Sector %d,%d Colonized.\n", s->land_x, s->land_y);
            notify(g.player, g.governor, buf);
          }
          planet.info[g.player - 1].resource += scrapval;
          planet.popn += crewval;
          planet.info[g.player - 1].destruct += destval;
          planet.info[g.player - 1].fuel += (int)fuelval;
          planet.info[g.player - 1].crystals += (int)xtalval;
          putsector(sect, planet, s->land_x, s->land_y);
        }
        putplanet(planet, Stars[s->storbits], (int)s->pnumorbits);
      }
      if (landed(s)) {
        g.out << "\nScrapped.\n";
      } else {
        g.out << "\nDestroyed.\n";
      }
      free(s);
    } else
      free(s);
}
