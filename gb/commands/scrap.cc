// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* scrap.c -- turn a ship to junk */

module;

import gblib;
import std.compat;

#include "gb/buffers.h"
#include "gb/land.h"
#include "gb/load.h"
#include "gb/races.h"

module commands;

namespace GB::commands {
void scrap(const command_t &argv, GameObj &g) {
  ap_t APcount = 1;
  Sector sect;
  Ship *s;
  shipnum_t shipno;
  shipnum_t nextshipno;
  int scrapval = 0;
  int destval = 0;
  int crewval = 0;
  int xtalval = 0;
  int troopval = 0;
  double fuelval = 0.0;

  if (argv.size() < 2) {
    g.out << "Scrap what?\n";
    return;
  }

  nextshipno = start_shiplist(g, argv[1]);

  while ((shipno = do_shiplist(&s, &nextshipno)))
    if (in_list(g.player, argv[1], *s, &nextshipno) &&
        authorized(g.governor, *s)) {
      if (s->max_crew && !s->popn) {
        notify(g.player, g.governor, "Can't scrap that ship - no crew.\n");
        free(s);
        continue;
      }
      if (s->whatorbits == ScopeLevel::LEVEL_UNIV) {
        continue;
      }
      if (!enufAP(g.player, g.governor, stars[s->storbits].AP[g.player - 1],
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
        const auto planet = getplanet(s->storbits, s->pnumorbits);
        if (landed(*s)) sect = getsector(planet, s->land_x, s->land_y);
      }
      std::optional<Ship> s2;
      if (docked(*s)) {
        s2 = getship(s->destshipno);
        if (!s2) {
          continue;
        }
        // TODO(jeffbailey): Changed from !s->whatorbits, which didn't make any
        // sense.
        if (!(s2->docked && s2->destshipno == s->number) &&
            s->whatorbits != ScopeLevel::LEVEL_SHIP) {
          g.out << "Warning, other ship not docked..\n";
          free(s);
          continue;
        }
      }

      scrapval = shipcost(*s) / 2 + s->resource;

      if (s->docked) {
        sprintf(buf, "%s: original cost: %ld\n", ship_to_string(*s).c_str(),
                shipcost(*s));
        notify(g.player, g.governor, buf);
        sprintf(buf, "         scrap value%s: %d rp's.\n",
                s->resource ? "(with stockpile) " : "", scrapval);
        notify(g.player, g.governor, buf);

        if (s->whatdest == ScopeLevel::LEVEL_SHIP &&
            s2->resource + scrapval > max_resource(*s2) &&
            s2->type != ShipType::STYPE_SHUTTLE) {
          scrapval = max_resource(*s2) - s2->resource;
          sprintf(buf, "(There is only room for %d resources.)\n", scrapval);
          notify(g.player, g.governor, buf);
        }
        if (s->fuel) {
          sprintf(buf, "Fuel recovery: %.0f.\n", s->fuel);
          notify(g.player, g.governor, buf);
          fuelval = s->fuel;
          if (s->whatdest == ScopeLevel::LEVEL_SHIP &&
              s2->fuel + fuelval > max_fuel(*s2)) {
            fuelval = max_fuel(*s2) - s2->fuel;
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
              s2->destruct + destval > max_destruct(*s2)) {
            destval = max_destruct(*s2) - s2->destruct;
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
                s2->troops + troopval > max_mil(*s2)) {
              troopval = max_mil(*s2) - s2->troops;
              sprintf(buf, "(There is only room for %d troops.)\n", troopval);
              notify(g.player, g.governor, buf);
            }
            crewval = s->popn;
            if (s->whatdest == ScopeLevel::LEVEL_SHIP &&
                s2->popn + crewval > max_crew(*s2)) {
              crewval = max_crew(*s2) - s2->popn;
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
                s2->crystals + xtalval > max_crystals(*s2)) {
              xtalval = max_crystals(*s2) - s2->crystals;
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
        deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
      else
        deductAPs(g, APcount, s->storbits);

      auto &race = races[g.player - 1];

      // TODO(jeffbailey): kill_ship gets and saves the ship, which looks like
      // it'll be overwritten maybe here?
      kill_ship(g.player, s);
      putship(s);
      if (docked(*s)) {
        s2->crystals += xtalval;
        rcv_fuel(*s2, (double)fuelval);
        rcv_destruct(*s2, destval);
        rcv_resource(*s2, scrapval);
        rcv_troops(*s2, troopval, race.mass);
        rcv_popn(*s2, crewval, race.mass);
        /* check for docking status in case scrapped ship is landed. Maarten */
        if (!(s->whatorbits == ScopeLevel::LEVEL_SHIP)) {
          s2->docked = 0; /* undock the surviving ship */
          s2->whatdest = ScopeLevel::LEVEL_UNIV;
          s2->destshipno = 0;
        }
        putship(&*s2);
      }

      if (s->whatorbits == ScopeLevel::LEVEL_PLAN) {
        auto planet = getplanet(s->storbits, s->pnumorbits);
        if (landed(*s)) {
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
        putplanet(planet, stars[s->storbits], s->pnumorbits);
      }
      if (landed(*s)) {
        g.out << "\nScrapped.\n";
      } else {
        g.out << "\nDestroyed.\n";
      }
      free(s);
    } else
      free(s);
}
}  // namespace GB::commands