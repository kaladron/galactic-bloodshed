// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include <boost/format.hpp>

import std;

#include "gb/GB_server.h"
#include "gb/defense.h"
#include "gb/ships.h"
#include "gb/shlmisc.h"

#include "gb/files_shl.h"

// Essentialy everything in this file can move into a Ship class.

/* can takeoff & land, is mobile, etc. */
unsigned short speed_rating(const Ship &s) { return s.max_speed; }

/* has an on/off switch */
bool has_switch(const Ship &s) { return Shipdata[s.type][ABIL_HASSWITCH]; }

/* can bombard planets */
bool can_bombard(const Ship &s) {
  return Shipdata[s.type][ABIL_GUNS] && (s.type != ShipType::STYPE_MINE);
}

/* can navigate */
bool can_navigate(const Ship &s) {
  return Shipdata[s.type][ABIL_SPEED] > 0 && s.type != ShipType::OTYPE_TERRA &&
         s.type != ShipType::OTYPE_VN;
}

/* can aim at things. */
bool can_aim(const Ship &s) {
  return s.type >= ShipType::STYPE_MIRROR && s.type <= ShipType::OTYPE_TRACT;
}

/* macros to get ship stats */
unsigned long armor(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? Shipdata[s.type][ABIL_ARMOR]
                                             : s.armor * (100 - s.damage) / 100;
}

long guns(const Ship &s) {
  return (s.guns == GTYPE_NONE) ? 0
                                : (s.guns == PRIMARY ? s.primary : s.secondary);
}

population_t max_crew(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY)
             ? Shipdata[s.type][ABIL_MAXCREW] - s.troops
             : s.max_crew - s.troops;
}

population_t max_mil(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY)
             ? Shipdata[s.type][ABIL_MAXCREW] - s.popn
             : s.max_crew - s.popn;
}

long max_resource(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? Shipdata[s.type][ABIL_CARGO]
                                             : s.max_resource;
}
int max_crystals(const Ship &) { return 127; }

long max_fuel(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? Shipdata[s.type][ABIL_FUELCAP]
                                             : s.max_fuel;
}

long max_destruct(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? Shipdata[s.type][ABIL_DESTCAP]
                                             : s.max_destruct;
}

long max_speed(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? Shipdata[s.type][ABIL_SPEED]
                                             : s.max_speed;
}

long shipcost(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY)
             ? 2 * s.build_cost * s.on + Shipdata[s.type][ABIL_COST]
             : s.build_cost;
}

double mass(const Ship &s) { return s.mass; }

long shipsight(const Ship &s) {
  return (s.type == ShipType::OTYPE_PROBE) || s.popn;
}

long retaliate(const Ship &s) { return s.retaliate; }

int size(const Ship &s) { return s.size; }

int shipbody(const Ship &s) { return s.size - s.max_hanger; }

long hanger(const Ship &s) { return s.max_hanger - s.hanger; }

long repair(const Ship &s) {
  return (s.type == ShipType::OTYPE_FACTORY) ? s.on : max_crew(s);
}

Shiplist::Iterator::Iterator(shipnum_t a) {
  auto tmpship = getship(a);
  if (tmpship) {
    elem = *tmpship;
  } else {
    elem = Ship{};
    elem.number = 0;
  }
}

Shiplist::Iterator &Shiplist::Iterator::operator++() {
  auto tmpship = getship(elem.nextship);
  if (tmpship) {
    elem = *tmpship;
  } else {
    elem = Ship{};
    elem.number = 0;
  }
  return *this;
}

int getdefense(const Ship &ship) {
  if (landed(ship)) {
    const auto p = getplanet(ship.storbits, ship.pnumorbits);
    const auto sect = getsector(p, ship.land_x, ship.land_y);
    return (2 * Defensedata[sect.condition]);
  }
  // No defense
  return 0;
}

bool laser_on(const Ship &ship) { return (ship.laser && ship.fire_laser); }

bool landed(const Ship &ship) {
  return (ship.whatdest == ScopeLevel::LEVEL_PLAN && ship.docked);
}

void capture_stuff(const Ship &ship, GameObj &g) {
  Shiplist shiplist(ship.ships);
  for (auto s : shiplist) {
    capture_stuff(s, g);  /* recursive call */
    s.owner = ship.owner; /* make sure he gets all of the ships landed on it */
    s.governor = ship.governor;
    putship(&s);
    g.out << ship_to_string(s) << " CAPTURED!\n";
  }
}

std::string ship_to_string(const Ship &s) {
  return str(boost::format("%c%lu %s [%d]") % Shipltrs[s.type] % s.number %
             s.name % s.owner);
}

double getmass(const Ship &s) {
  return (1.0 + MASS_ARMOR * s.armor + MASS_SIZE * (s.size - s.max_hanger) +
          MASS_HANGER * s.max_hanger + MASS_GUNS * s.primary * s.primtype +
          MASS_GUNS * s.secondary * s.sectype);
}

unsigned int ship_size(const Ship &s) {
  double size = 1.0 + SIZE_GUNS * s.primary + SIZE_GUNS * s.secondary +
                SIZE_CREW * s.max_crew + SIZE_RESOURCE * s.max_resource +
                SIZE_FUEL * s.max_fuel + SIZE_DESTRUCT * s.max_destruct +
                s.max_hanger;
  return (std::floor(size));
}

double cost(const Ship &s) {
  /* compute how much it costs to build this ship */
  double factor = 0.0;
  factor += (double)Shipdata[s.build_type][ABIL_COST];
  factor += GUN_COST * (double)s.primary;
  factor += GUN_COST * (double)s.secondary;
  factor += CREW_COST * (double)s.max_crew;
  factor += CARGO_COST * (double)s.max_resource;
  factor += FUEL_COST * (double)s.max_fuel;
  factor += AMMO_COST * (double)s.max_destruct;
  factor +=
      SPEED_COST * (double)s.max_speed * (double)sqrt((double)s.max_speed);
  factor += HANGER_COST * (double)s.max_hanger;
  factor += ARMOR_COST * (double)s.armor * (double)sqrt((double)s.armor);
  factor += CEW_COST * (double)(s.cew * s.cew_range);
  /* additional advantages/disadvantages */

  double advantage = 0.0;
  advantage += 0.5 * !!s.hyper_drive.has;
  advantage += 0.5 * !!s.laser;
  advantage += 0.5 * !!s.cloak;
  advantage += 0.5 * !!s.mount;

  factor *= sqrt(1.0 + advantage);
  return factor;
}

namespace {
void system_cost(double *advantage, double *disadvantage, int value, int base) {
  double factor;

  factor = (((double)value + 1.0) / (base + 1.0)) - 1.0;
  if (factor >= 0.0)
    *advantage += factor;
  else
    *disadvantage -= factor;
}
}  // namespace

double complexity(const Ship &s) {
  double advantage = 0.;
  double disadvantage = 0.;

  system_cost(&advantage, &disadvantage, (int)(s.primary),
              Shipdata[s.build_type][ABIL_GUNS]);
  system_cost(&advantage, &disadvantage, (int)(s.secondary),
              Shipdata[s.build_type][ABIL_GUNS]);
  system_cost(&advantage, &disadvantage, (int)(s.max_crew),
              Shipdata[s.build_type][ABIL_MAXCREW]);
  system_cost(&advantage, &disadvantage, (int)(s.max_resource),
              Shipdata[s.build_type][ABIL_CARGO]);
  system_cost(&advantage, &disadvantage, (int)(s.max_fuel),
              Shipdata[s.build_type][ABIL_FUELCAP]);
  system_cost(&advantage, &disadvantage, (int)(s.max_destruct),
              Shipdata[s.build_type][ABIL_DESTCAP]);
  system_cost(&advantage, &disadvantage, (int)(s.max_speed),
              Shipdata[s.build_type][ABIL_SPEED]);
  system_cost(&advantage, &disadvantage, (int)(s.max_hanger),
              Shipdata[s.build_type][ABIL_HANGER]);
  system_cost(&advantage, &disadvantage, (int)(s.armor),
              Shipdata[s.build_type][ABIL_ARMOR]);
  /* additional advantages/disadvantages */

  // TODO(jeffbailey): document this function in English.
  double factor = sqrt((1.0 + advantage) * exp(-(double)disadvantage / 10.0));
  double tmp = COMPLEXITY_FACTOR * (factor - 1.0) /
                   sqrt((double)(Shipdata[s.build_type][ABIL_TECH] + 1)) +
               1.0;
  factor = tmp * tmp;
  return (factor * (double)Shipdata[s.build_type][ABIL_TECH]);
}

bool testship(const Ship &s, const player_t playernum,
              const governor_t governor) {
  char buf[255];
  if (!s.alive) {
    sprintf(buf, "%s has been destroyed.\n", ship_to_string(s).c_str());
    notify(playernum, governor, buf);
    return true;
  }

  if (s.owner != playernum || !authorized(governor, s)) {
    DontOwnErr(playernum, governor, s.number);
    return true;
  }

  if (!s.active) {
    sprintf(buf, "%s is irradiated %d%% and inactive.\n",
            ship_to_string(s).c_str(), s.rad);
    notify(playernum, governor, buf);
    return true;
  }

  return false;
}
