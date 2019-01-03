// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "ships.h"
#include "files_shl.h"

/* can takeoff & land, is mobile, etc. */
unsigned short speed_rating(Ship *s) { return (s)->max_speed; }

/* has an on/off switch */
bool has_switch(Ship *s) { return Shipdata[(s)->type][ABIL_HASSWITCH]; }

/* can bombard planets */
bool can_bombard(Ship *s) {
  return Shipdata[(s)->type][ABIL_GUNS] && ((s)->type != ShipType::STYPE_MINE);
}

/* can navigate */
bool can_navigate(Ship *s) {
  return Shipdata[(s)->type][ABIL_SPEED] > 0 &&
         (s)->type != ShipType::OTYPE_TERRA && (s)->type != ShipType::OTYPE_VN;
}

/* can aim at things. */
bool can_aim(Ship *s) {
  return (s)->type >= ShipType::STYPE_MIRROR &&
         (s)->type <= ShipType::OTYPE_TRACT;
}

/* macros to get ship stats */
unsigned long Armor(Ship *s) {
  return ((s)->type == ShipType::OTYPE_FACTORY)
             ? Shipdata[(s)->type][ABIL_ARMOR]
             : (s)->armor * (100 - (s)->damage) / 100;
}

long Guns(Ship *s) {
  return ((s)->guns == GTYPE_NONE)
             ? 0
             : ((s)->guns == PRIMARY ? (s)->primary : (s)->secondary);
}

long Max_crew(Ship *s) {
  return ((s)->type == ShipType::OTYPE_FACTORY)
             ? Shipdata[(s)->type][ABIL_MAXCREW] - (s)->troops
             : (s)->max_crew - (s)->troops;
}

long Max_mil(Ship *s) {
  return ((s)->type == ShipType::OTYPE_FACTORY)
             ? Shipdata[(s)->type][ABIL_MAXCREW] - (s)->popn
             : (s)->max_crew - (s)->popn;
}

long Max_resource(Ship *s) {
  return ((s)->type == ShipType::OTYPE_FACTORY)
             ? Shipdata[(s)->type][ABIL_CARGO]
             : (s)->max_resource;
}
int Max_crystals(Ship *) { return 127; }

long Max_fuel(Ship *s) {
  return ((s)->type == ShipType::OTYPE_FACTORY)
             ? Shipdata[(s)->type][ABIL_FUELCAP]
             : (s)->max_fuel;
}

long Max_destruct(Ship *s) {
  return ((s)->type == ShipType::OTYPE_FACTORY)
             ? Shipdata[(s)->type][ABIL_DESTCAP]
             : (s)->max_destruct;
}

long Max_speed(Ship *s) {
  return ((s)->type == ShipType::OTYPE_FACTORY)
             ? Shipdata[(s)->type][ABIL_SPEED]
             : (s)->max_speed;
}

long Cost(Ship *s) {
  return ((s)->type == ShipType::OTYPE_FACTORY)
             ? 2 * (s)->build_cost * (s)->on + Shipdata[(s)->type][ABIL_COST]
             : (s)->build_cost;
}

double Mass(Ship *s) { return (s)->mass; }

long Sight(Ship *s) {
  return ((s)->type == ShipType::OTYPE_PROBE) || (s)->popn;
}

long Retaliate(Ship *s) { return (s)->retaliate; }

int Size(Ship *s) { return (s)->size; }

int Body(Ship *s) { return (s)->size - (s)->max_hanger; }

long Hanger(Ship *s) { return (s)->max_hanger - (s)->hanger; }

long Repair(Ship *s) {
  return ((s)->type == ShipType::OTYPE_FACTORY) ? (s)->on : Max_crew(s);
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
