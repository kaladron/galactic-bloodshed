// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* VN.c -- assorted Von Neumann machine code */

import gblib;
import std.compat;

#include "gb/VN.h"

#include <cstdlib>
#include <strings.h>

#include "gb/doturn.h"
#include "gb/load.h"
#include "gb/max.h"
#include "gb/ships.h"
#include "gb/tele.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

static void order_berserker(Ship &);
static void order_VN(Ship &);

/*  do_VN() -- called by doship() */
void do_VN(Ship &ship) {
  if (!landed(ship)) {
    // Doing other things
    if (!ship.special.mind.busy) return;

    // we were just built & launched
    if (ship.type == ShipType::OTYPE_BERS)
      order_berserker(ship);
    else
      order_VN(ship);
    return;
  }

  Stinfo[ship.storbits][ship.pnumorbits].inhab = 1;

  /* launch if no assignment */
  if (!ship.special.mind.busy) {
    if (ship.fuel >= (double)ship.max_fuel) {
      ship.xpos = stars[ship.storbits].xpos +
                  planets[ship.storbits][ship.pnumorbits]->xpos +
                  int_rand(-10, 10);
      ship.ypos = stars[ship.storbits].ypos +
                  planets[ship.storbits][ship.pnumorbits]->ypos +
                  int_rand(-10, 10);
      ship.docked = 0;
      ship.whatdest = ScopeLevel::LEVEL_UNIV;
    }
    return;
  }

  /* we have an assignment.  Since we are landed, this means
     we are engaged in building up resources/fuel. */
  /* steal resources from other players */
  /* permute list of people to steal from */
  std::array<int, MAXPLAYERS + 1> nums;
  for (int i = 1; i <= Num_races; i++) nums[i] = i;
  for (int i = 1; i <= Num_races; i++) {
    int f = int_rand(1, Num_races);
    std::swap(nums[i], nums[f]);
  }

  auto &p = planets[ship.storbits][ship.pnumorbits];

  // Loop through permuted vector until someone has resources on
  // this planet to steal

  player_t f = 0;
  for (player_t i = 1; i <= Num_races; i++)
    if (p->info[nums[i] - 1].resource) f = nums[i];

  // No resources to steal
  if (f == 0) return;

  // Steal the resources

  auto prod = std::min(p->info[f - 1].resource,
                       Shipdata[ShipType::OTYPE_VN][ABIL_COST]);
  p->info[f - 1].resource -= prod;

  std::string buf;

  if (ship.type == ShipType::OTYPE_VN) {
    rcv_resource(ship, prod);
    buf = std::format("{0} resources stolen from [{1}] by {2}{3} at {4}.", prod,
                      f, Shipltrs[ShipType::OTYPE_VN], ship.number,
                      prin_ship_orbits(&ship));
  } else if (ship.type == ShipType::OTYPE_BERS) {
    rcv_destruct(ship, prod);
    buf = std::format("{0} resources stolen from [{1}] by {2}{3} at {4}.", prod,
                      f, Shipltrs[ShipType::OTYPE_BERS], ship.number,
                      prin_ship_orbits(&ship));
  }

  push_telegram_race(f, buf);
  if (f != ship.owner) push_telegram(ship.owner, ship.governor, buf);
}

static void order_berserker(Ship &ship) {
  /* give berserkers a mission - send to planet of offending player and bombard
   * it */
  ship.bombard = 1;
  ship.special.mind.target = VN_brain.Most_mad; /* who to attack */
  ship.whatdest = ScopeLevel::LEVEL_PLAN;
  if (random() & 01)
    ship.deststar = Sdata.VN_index1[ship.special.mind.target - 1];
  else
    ship.deststar = Sdata.VN_index2[ship.special.mind.target - 1];
  ship.destpnum = int_rand(0, stars[ship.deststar].numplanets - 1);
  if (ship.hyper_drive.has && ship.mounted) {
    ship.hyper_drive.on = 1;
    ship.hyper_drive.ready = 1;
    ship.special.mind.busy = 1;
  }
}

static void order_VN(Ship &ship) {
  int min = 0;
  int min2 = 0;

  /* find closest star */
  for (auto s = 0; s < Sdata.numstars; s++)
    if (s != ship.storbits &&
        Distsq(stars[s].xpos, stars[s].ypos, ship.xpos, ship.ypos) <
            Distsq(stars[min].xpos, stars[min].ypos, ship.xpos, ship.ypos)) {
      min2 = min;
      min = s;
    }

  /* don't go there if we have a choice,
     and we have VN's there already */
  if (isset(stars[min].inhabited, 1U))
    if (isset(stars[min2].inhabited, 1U))
      ship.deststar = int_rand(0, (int)Sdata.numstars - 1);
    else
      ship.deststar = min2; /* 2nd closest star */
  else
    ship.deststar = min;

  if (stars[ship.deststar].numplanets) {
    ship.destpnum = int_rand(0, stars[ship.deststar].numplanets - 1);
    ship.whatdest = ScopeLevel::LEVEL_PLAN;
    ship.special.mind.busy = 1;
  } else {
    /* no good; find someplace else. */
    ship.special.mind.busy = 0;
  }
  ship.speed = Shipdata[ShipType::OTYPE_VN][ABIL_SPEED];
}

/*  planet_doVN() -- called by doplanet() */
void planet_doVN(Ship *ship, Planet &planet, SectorMap &smap) {
  int j;
  int oldres;
  int xa;
  int ya;
  int dum;
  int prod;

  if (landed(*ship)) {
    if (ship->type == ShipType::OTYPE_VN && ship->special.mind.busy) {
      /* first try and make some resources(VNs) by ourselves.
         more might be stolen in doship */
      auto &s = smap.get(ship->land_x, ship->land_y);
      if (!(oldres = s.resource)) {
        /* move to another sector */
        xa = int_rand(-1, 1);
        ship->land_x = mod((int)(ship->land_x) + xa, planet.Maxx, dum);
        ya = (ship->land_y == 0)
                 ? 1
                 : ((ship->land_y == (planet.Maxy - 1)) ? -1 : int_rand(-1, 1));
        ship->land_y += ya;
      } else {
        /* mine the sector */
        s.resource *= VN_RES_TAKE;
        prod =
            oldres - s.resource; /* poor way for a player to mine resources */
        if (ship->type == ShipType::OTYPE_VN)
          rcv_resource(*ship, prod);
        else if (ship->type == ShipType::OTYPE_BERS)
          rcv_destruct(*ship, 5 * prod);
        rcv_fuel(*ship, (double)prod);
      }
      /* now try to construct another machine */
      ShipType shipbuild = (VN_brain.Total_mad > 100 && random() & 01)
                               ? ShipType::OTYPE_BERS
                               : ShipType::OTYPE_VN;
      if (ship->resource >= Shipdata[shipbuild][ABIL_COST]) {
        Ship *s2;
        int n;
        int numVNs;
        /* construct as many VNs as possible */
        numVNs = ship->resource / Shipdata[shipbuild][ABIL_COST];
        for (j = 1; j <= numVNs; j++) {
          use_resource(*ship, Shipdata[shipbuild][ABIL_COST]);
          /* must change size of ships pointer */
          ++Num_ships;
          ships = (Ship **)realloc(ships, (Num_ships + 1) * sizeof(Ship *));
          ships[Num_ships] = (Ship *)malloc(sizeof(Ship));
          s2 = ships[Num_ships];
          bzero((char *)s2, sizeof(Ship));
          s2->nextship = planet.ships;
          planet.ships = Num_ships;
          s2->number = Num_ships;
          s2->whatorbits = ScopeLevel::LEVEL_PLAN;
          s2->storbits = ship->storbits;
          s2->pnumorbits = ship->pnumorbits;
          s2->docked = 1;
          s2->land_x = ship->land_x;
          s2->land_y = ship->land_y;
          s2->whatdest = ship->whatdest;
          s2->deststar = ship->deststar;
          s2->destpnum = ship->destpnum;
          s2->xpos = ship->xpos;
          s2->ypos = ship->ypos;
          s2->type = shipbuild;
          s2->mode = 0;
          s2->armor = ship->armor + 1;
          s2->guns = Shipdata[shipbuild][ABIL_PRIMARY] ? PRIMARY : GTYPE_NONE;
          s2->primary = Shipdata[shipbuild][ABIL_GUNS];
          s2->primtype = Shipdata[shipbuild][ABIL_PRIMARY];
          s2->secondary = Shipdata[shipbuild][ABIL_GUNS];
          s2->sectype = Shipdata[shipbuild][ABIL_SECONDARY];
          s2->max_crew = Shipdata[shipbuild][ABIL_MAXCREW];
          s2->max_resource = Shipdata[shipbuild][ABIL_CARGO];
          s2->max_fuel = Shipdata[shipbuild][ABIL_FUELCAP];
          s2->max_destruct = Shipdata[shipbuild][ABIL_DESTCAP];
          s2->max_speed = Shipdata[shipbuild][ABIL_SPEED];
          s2->size = ship_size(*s2);
          s2->base_mass = getmass(*s2);
          s2->mass = s2->base_mass;
          s2->alive = 1;
          if (shipbuild == ShipType::OTYPE_BERS) {
            /* special.mind.target = person killed the most VN's */
            s2->special.mind.target = VN_brain.Most_mad;
            sprintf(s2->name, "%x", s2->special.mind.target);
            s2->speed = Shipdata[ShipType::OTYPE_BERS][ABIL_SPEED];
            s2->tech = ship->tech + 100.0;
            s2->bombard = 1;
            s2->protect.self = 1;
            s2->protect.planet = 1;
            s2->armor += 10; /* give 'em some armor */
            s2->active = 1;
            s2->owner = 1;
            s2->governor = 0;
            s2->special.mind.progenitor = ship->special.mind.progenitor;
            s2->fuel = 5 * ship->fuel; /* give 'em some fuel */
            s2->retaliate = s2->primary;
            s2->destruct = 500;
            ship->fuel *= 0.5; /* lose some fuel */
            s2->hyper_drive.has = 1;
            s2->hyper_drive.on = 1;
            s2->hyper_drive.ready = 1;
            s2->hyper_drive.charge = 0;
            s2->mounted = 1;
            auto buf = std::format("{0} constructed {1}.",
                                   ship_to_string(*ship), ship_to_string(*s2));
            push_telegram(ship->owner, ship->governor, buf);
            s2->special.mind.tampered = 0;
          } else {
            s2->tech = ship->tech + 20.0;
            n = int_rand(3, std::min(10, SHIP_NAMESIZE)); /* for name */
            s2->name[n] = '\0';
            while (n--) s2->name[n] = (random() & 01) + '0';
            s2->owner = 1;
            s2->governor = 0;
            s2->active = 1;
            s2->speed = Shipdata[ShipType::OTYPE_VN][ABIL_SPEED];
            s2->bombard = 0;
            s2->fuel = 0.5 * ship->fuel;
            ship->fuel *= 0.5;
          }
          s2->special.mind.busy = 0;
          s2->special.mind.progenitor = ship->special.mind.progenitor;
          s2->special.mind.generation = ship->special.mind.generation + 1;
          ship->special.mind.busy = random() & 01;
        }
      }
    } else { /* orbiting a planet */
      if (ship->special.mind.busy) {
        if (ship->whatdest == ScopeLevel::LEVEL_PLAN &&
            ship->deststar == ship->storbits &&
            ship->destpnum == ship->pnumorbits) {
          if (planet.type == PlanetType::GASGIANT)
            ship->special.mind.busy = 0;
          else {
            /* find a place on the planet to land */
            bool found = false;
            for (auto shuffled = smap.shuffle(); auto &sector_wrap : shuffled) {
              Sector &sect = sector_wrap;
              if (sect.resource == 0) continue;
              found = true;
              ship->docked = 1;
              ship->whatdest = ScopeLevel::LEVEL_PLAN;
              ship->deststar = ship->storbits;
              ship->destpnum = ship->pnumorbits;
              ship->xpos = stars[ship->storbits].xpos + planet.xpos;
              ship->ypos = stars[ship->storbits].ypos + planet.ypos;
              ship->land_x = sect.x;
              ship->land_y = sect.y;
              ship->special.mind.busy = 1;
            }
            if (!found) ship->special.mind.busy = 0;
          }
        }
      }
    }
  }
}
