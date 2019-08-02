// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/ships.h"

#include "gb/files_shl.h"

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
