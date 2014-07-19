// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include <stdint.h>

#include "vars.h"

#ifndef SHIPS_H
#define SHIPS_H

#define LIGHT 1
#define MEDIUM 2
#define HEAVY 3
#define NONE 0
#define PRIMARY 1
#define SECONDARY 2

#define STYPE_POD 0
#define STYPE_SHUTTLE 1
#define STYPE_CARRIER 2
#define STYPE_DREADNT 3
#define STYPE_BATTLE 4
#define STYPE_INTCPT 5
#define STYPE_CRUISER 6
#define STYPE_DESTROYER 7
#define STYPE_FIGHTER 8
#define STYPE_EXPLORER 9
#define STYPE_HABITAT 10
#define STYPE_STATION 11
#define STYPE_OAP 12
#define STYPE_CARGO 13
#define STYPE_TANKER 14
#define STYPE_GOD 15
#define STYPE_MINE 16
#define STYPE_MIRROR 17
#define OTYPE_STELE 18
#define OTYPE_GTELE 19
#define OTYPE_TRACT 20
#define OTYPE_AP 21
#define OTYPE_CANIST 22
#define OTYPE_GREEN 23
#define OTYPE_VN 24
#define OTYPE_BERS 25
#define OTYPE_GOV 26
#define OTYPE_OMCL 27
#define OTYPE_TOXWC 28
#define OTYPE_PROBE 29
#define OTYPE_GR 30
#define OTYPE_FACTORY 31
#define OTYPE_TERRA 32
#define OTYPE_BERSCTLC 33
#define OTYPE_AUTOFAC 34
#define OTYPE_TRANSDEV 35
#define STYPE_MISSILE 36
#define OTYPE_PLANDEF 37
#define OTYPE_QUARRY 38
#define OTYPE_PLOW 39
#define OTYPE_DOME 40
#define OTYPE_WPLANT 41
#define OTYPE_PORT 42
#define OTYPE_ABM 43
#define OTYPE_AFV 44
#define OTYPE_BUNKER 45
#define STYPE_LANDER 46

#define ABIL_TECH 0
#define ABIL_CARGO 1
#define ABIL_HANGER 2
#define ABIL_DESTCAP 3
#define ABIL_GUNS 4
#define ABIL_PRIMARY 5
#define ABIL_SECONDARY 6
#define ABIL_FUELCAP 7
#define ABIL_MAXCREW 8
#define ABIL_ARMOR 9
#define ABIL_COST 10
#define ABIL_MOUNT 11
#define ABIL_JUMP 12
#define ABIL_CANLAND 13
#define ABIL_HASSWITCH 14
#define ABIL_SPEED 15
#define ABIL_DAMAGE 16
#define ABIL_BUILD 17
#define ABIL_CONSTRUCT 18
#define ABIL_MOD 19
#define ABIL_LASER 20
#define ABIL_CEW 21
#define ABIL_CLOAK 22
#define ABIL_GOD 23 /* only diety can build these objects */
#define ABIL_PROGRAMMED 24
#define ABIL_PORT 25
#define ABIL_REPAIR 26
#define ABIL_MAINTAIN 27

#define NUMSTYPES (STYPE_LANDER + 1)
#define NUMABILS (ABIL_MAINTAIN + 1)

#define SHIP_NAMESIZE 18

typedef struct ship shiptype;
typedef struct place placetype;

struct ship {
  unsigned short number;         /* ship knows its own number */
  unsigned char owner;           /* owner of ship */
  unsigned char governor;        /* subordinate that controls the ship */
  char name[SHIP_NAMESIZE];      /* name of ship (optional) */
  char shipclass[SHIP_NAMESIZE]; /* shipclass of ship - designated by players */

  unsigned char race; /* race type - used when you gain alien
                         ships during revolts and whatnot - usually
                         equal to owner */
  double xpos;
  double ypos;
  double fuel;
  double mass;
  unsigned char land_x, land_y;

  unsigned short destshipno; /* destination ship # */
  unsigned short nextship;   /* next ship in linked list */
  unsigned short ships;      /* ships landed on it */

  unsigned char armor;
  unsigned short size;

  unsigned short max_crew;
  unsigned short max_resource;
  unsigned short max_destruct;
  unsigned short max_fuel;
  unsigned short max_speed;
  unsigned short build_type; /* for factories - type of ship it makes */
  unsigned short build_cost;

  double base_mass;
  double tech;       /* engineering technology rating */
  double complexity; /* complexity rating */

  unsigned short destruct; /* stuff it's carrying */
  unsigned short resource;
  unsigned short popn;   /* crew */
  unsigned short troops; /* marines */
  unsigned short crystals;

  /* special ship functions (10 bytes) */
  union {
    struct {                  /* if the ship is a Space Mirror */
      unsigned short shipno;  /* aimed at what ship */
      unsigned char snum;     /* aimed at what star */
      char intensity;         /* intensity of aiming */
      unsigned char pnum;     /* aimed at what planet */
      levels_t level;         /* aimed at what level */
      unsigned char dummy[4]; /* unused bytes */
    } aimed_at;
    struct {                    /* VNs and berserkers */
      unsigned char progenitor; /* the originator of the strain */
      unsigned char target;     /* who to kill (for Berserkers) */
      unsigned char generation;
      unsigned char busy;     /* currently occupied */
      unsigned char tampered; /* recently tampered with? */
      unsigned char who_killed;
      unsigned char dummy[4];
    } mind;
    struct { /* spore pods */
      unsigned char decay;
      unsigned char temperature;
      unsigned char dummy[8];
    } pod;
    struct { /* dust canisters, greenhouse gases */
      unsigned char count;
      unsigned char dummy[9];
    } timer;
    struct { /* missiles */
      unsigned char x;
      unsigned char y;
      unsigned char scatter;
      unsigned char dummy[7];
    } impact;
    struct { /* mines */
      unsigned short radius;
      unsigned char dummy[8];
    } trigger;
    struct { /* terraformers */
      unsigned char index;
      unsigned char dummy[9];
    } terraform;
    struct { /* AVPM */
      unsigned short target;
      unsigned char dummy[8];
    } transport;
    struct { /* toxic waste containers */
      unsigned char toxic;
      unsigned char dummy[9];
    } waste;
  } special;

  short who_killed; /* who killed the ship */

  struct {
    unsigned on : 1;      /* toggles navigate mode */
    unsigned speed : 4;   /* speed for navigate command */
    unsigned turns : 15;  /* number turns left in maneuver */
    unsigned bearing : 9; /* course */
    unsigned dummy : 3;
  } navigate;

  struct {
    double maxrng;       /* maximum range for autoshoot */
    unsigned on : 1;     /* toggle on/off */
    unsigned planet : 1; /* planet defender */
    unsigned self : 1;   /* retaliate if attacked */
    unsigned evade : 1;  /* evasive action */
    unsigned ship : 14;  /* ship it is protecting */
    unsigned dummy : 6;
  } protect;

  /* special systems */
  unsigned char mount; /* has a crystal mount */
  struct {
    unsigned char charge;
    unsigned ready : 1;
    unsigned on : 1;
    unsigned has : 1;
    unsigned dummy : 5;
  } hyper_drive;
  unsigned char cew;        /* CEW strength */
  unsigned short cew_range; /* CEW (confined-energy-weapon) range */
  unsigned char cloak;      /* has cloaking device */
  unsigned char laser;      /* has a laser */
  unsigned char focus;      /* focused laser mode */
  unsigned char fire_laser; /* retaliation strength for lasers */

  unsigned char storbits;   /* what star # orbits */
  unsigned char deststar;   /* destination star */
  unsigned char destpnum;   /* destination planet */
  unsigned char pnumorbits; /* # of planet if orbiting */
  levels_t whatdest;        /* where going (same as Dir) */
  levels_t whatorbits;      /* where orbited (same as Dir) */

  unsigned char damage; /* amt of damage */
  unsigned char rad;    /* radiation level */
  unsigned char retaliate;
  unsigned short target;

  unsigned char type;  /* what type ship is */
  unsigned char speed; /* what speed to travel at 0-9 */

  unsigned active : 1; /* tells whether the ship is active */
  unsigned alive : 1;  /* ship is alive */
  unsigned mode : 1;
  unsigned bombard : 1;  /* bombard planet we're orbiting */
  unsigned mounted : 1;  /* has a crystal mounted */
  unsigned cloaked : 1;  /* is cloaked ship */
  unsigned sheep : 1;    /* is under influence of mind control */
  unsigned docked : 1;   /* is landed on a planet or docked */
  unsigned notified : 1; /* has been notified of something */
  unsigned examined : 1; /* has been examined */
  unsigned on : 1;       /* on or off */
  unsigned dummy4 : 5;

  unsigned char merchant; /* this contains the route number */
  unsigned char guns;     /* current gun system which is active */
  unsigned char primary;  /* describe primary gun system */
  unsigned char primtype;
  unsigned char secondary; /* describe secondary guns */
  unsigned char sectype;

  unsigned short hanger;     /* amount of hanger space used */
  unsigned short max_hanger; /* total hanger space */
};

struct place { /* used in function return for finding place */
  uint8_t snum;
  uint8_t pnum;
  unsigned short shipno;
  shiptype *shipptr;
  levels_t level;    /* .level: same as Dir */
  unsigned char err; /* if error */
};

/* can takeoff & land, is mobile, etc. */
#define speed_rating(s) ((s)->max_speed)

/* has an on/off switch */
#define has_switch(s) (Shipdata[(s)->type][ABIL_HASSWITCH])

/* can bombard planets */
#define can_bombard(s)                                                         \
  (Shipdata[(s)->type][ABIL_GUNS] && ((s)->type != STYPE_MINE))

/* can navigate */
#define can_navigate(s)                                                        \
  (Shipdata[(s)->type][ABIL_SPEED] > 0 &&                                      \
   (s)->type != OTYPE_TERRA &&(s)->type != OTYPE_VN)

/* can aim at things. */
#define can_aim(s) ((s)->type >= STYPE_MIRROR &&(s)->type <= OTYPE_TRACT)

/* macros to get ship stats */
#define Armor(s)                                                               \
  (((s)->type == OTYPE_FACTORY) ? Shipdata[(s)->type][ABIL_ARMOR]              \
                                : (s)->armor *(100 - (s)->damage) / 100)
#define Guns(s)                                                                \
  (((s)->guns == NONE) ? 0 : ((s)->guns == PRIMARY ? (s)->primary              \
                                                   : (s)->secondary))
#define Max_crew(s)                                                            \
  (((s)->type == OTYPE_FACTORY)                                                \
       ? Shipdata[(s)->type][ABIL_MAXCREW] - (s)->troops                       \
       : (s)->max_crew - (s)->troops)
#define Max_mil(s)                                                             \
  (((s)->type == OTYPE_FACTORY)                                                \
       ? Shipdata[(s)->type][ABIL_MAXCREW] - (s)->popn                         \
       : (s)->max_crew - (s)->popn)
#define Max_resource(s)                                                        \
  (((s)->type == OTYPE_FACTORY) ? Shipdata[(s)->type][ABIL_CARGO]              \
                                : (s)->max_resource)
#define Max_crystals(s) (127)
#define Max_fuel(s)                                                            \
  (((s)->type == OTYPE_FACTORY) ? Shipdata[(s)->type][ABIL_FUELCAP]            \
                                : (s)->max_fuel)
#define Max_destruct(s)                                                        \
  (((s)->type == OTYPE_FACTORY) ? Shipdata[(s)->type][ABIL_DESTCAP]            \
                                : (s)->max_destruct)
#define Max_speed(s)                                                           \
  (((s)->type == OTYPE_FACTORY) ? Shipdata[(s)->type][ABIL_SPEED]              \
                                : (s)->max_speed)
#define Cost(s)                                                                \
  (((s)->type == OTYPE_FACTORY)                                                \
       ? 2 * (s)->build_cost *(s)->on + Shipdata[(s)->type][ABIL_COST]         \
       : (s)->build_cost)
#define Mass(s) ((s)->mass)
#define Sight(s) (((s)->type == OTYPE_PROBE) || (s)->popn)
#define Retaliate(s) ((s)->retaliate)
#define Size(s) ((s)->size)
#define Body(s) ((s)->size - (s)->max_hanger)
#define Hanger(s) ((s)->max_hanger - (s)->hanger)
#define Repair(s) (((s)->type == OTYPE_FACTORY) ? (s)->on : Max_crew(s))

extern const long Shipdata[NUMSTYPES][NUMABILS];
extern const char Shipltrs[];
extern const char *Shipnames[];
extern int ShipVector[NUMSTYPES];

extern shiptype **ships;

#endif // SHIPS_H
