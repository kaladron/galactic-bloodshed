// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "ships.h"

/* can takeoff & land, is mobile, etc. */
unsigned short speed_rating(shiptype *s) { return (s)->max_speed; }

/* has an on/off switch */
bool has_switch(shiptype *s) { return Shipdata[(s)->type][ABIL_HASSWITCH]; }

/* can bombard planets */
bool can_bombard(shiptype *s) {
  return Shipdata[(s)->type][ABIL_GUNS] && ((s)->type != STYPE_MINE);
}

/* can navigate */
bool can_navigate(shiptype *s) {
  return Shipdata[(s)->type][ABIL_SPEED] > 0 && (s)->type != OTYPE_TERRA &&
         (s)->type != OTYPE_VN;
}

/* can aim at things. */
bool can_aim(shiptype *s) {
  return (s)->type >= STYPE_MIRROR && (s)->type <= OTYPE_TRACT;
}

/* macros to get ship stats */
unsigned long Armor(shiptype *s) {
  return ((s)->type == OTYPE_FACTORY) ? Shipdata[(s)->type][ABIL_ARMOR]
                                      : (s)->armor * (100 - (s)->damage) / 100;
}

long Guns(shiptype *s) {
  return ((s)->guns == GTYPE_NONE)
             ? 0
             : ((s)->guns == PRIMARY ? (s)->primary : (s)->secondary);
}

long Max_crew(shiptype *s) {
  return ((s)->type == OTYPE_FACTORY)
             ? Shipdata[(s)->type][ABIL_MAXCREW] - (s)->troops
             : (s)->max_crew - (s)->troops;
}

long Max_mil(shiptype *s) {
  return ((s)->type == OTYPE_FACTORY)
             ? Shipdata[(s)->type][ABIL_MAXCREW] - (s)->popn
             : (s)->max_crew - (s)->popn;
}

long Max_resource(shiptype *s) {
  return ((s)->type == OTYPE_FACTORY) ? Shipdata[(s)->type][ABIL_CARGO]
                                      : (s)->max_resource;
}
int Max_crystals(shiptype *s) { return 127; }

long Max_fuel(shiptype *s) {
  return ((s)->type == OTYPE_FACTORY) ? Shipdata[(s)->type][ABIL_FUELCAP]
                                      : (s)->max_fuel;
}

long Max_destruct(shiptype *s) {
  return ((s)->type == OTYPE_FACTORY) ? Shipdata[(s)->type][ABIL_DESTCAP]
                                      : (s)->max_destruct;
}

long Max_speed(shiptype *s) {
  return ((s)->type == OTYPE_FACTORY) ? Shipdata[(s)->type][ABIL_SPEED]
                                      : (s)->max_speed;
}

long Cost(shiptype *s) {
  return ((s)->type == OTYPE_FACTORY)
             ? 2 * (s)->build_cost * (s)->on + Shipdata[(s)->type][ABIL_COST]
             : (s)->build_cost;
}

double Mass(shiptype *s) { return (s)->mass; }

long Sight(shiptype *s) { return ((s)->type == OTYPE_PROBE) || (s)->popn; }

long Retaliate(shiptype *s) { return (s)->retaliate; }

int Size(shiptype *s) { return (s)->size; }

int Body(shiptype *s) { return (s)->size - (s)->max_hanger; }

long Hanger(shiptype *s) { return (s)->max_hanger - (s)->hanger; }

long Repair(shiptype *s) {
  return ((s)->type == OTYPE_FACTORY) ? (s)->on : Max_crew(s);
}
