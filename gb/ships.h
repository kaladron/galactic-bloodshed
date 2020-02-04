// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef SHIPS_H
#define SHIPS_H

#include "gb/vars.h"

enum guntype_t { GTYPE_NONE, GTYPE_LIGHT, GTYPE_MEDIUM, GTYPE_HEAVY };

#define PRIMARY 1
#define SECONDARY 2

enum ShipType : int {
  STYPE_POD,
  STYPE_SHUTTLE,
  STYPE_CARRIER,
  STYPE_DREADNT,
  STYPE_BATTLE,
  STYPE_INTCPT,
  STYPE_CRUISER,
  STYPE_DESTROYER,
  STYPE_FIGHTER,
  STYPE_EXPLORER,
  STYPE_HABITAT,
  STYPE_STATION,
  STYPE_OAP,
  STYPE_CARGO,
  STYPE_TANKER,
  STYPE_GOD,
  STYPE_MINE,
  STYPE_MIRROR,
  OTYPE_STELE,
  OTYPE_GTELE,
  OTYPE_TRACT,
  OTYPE_AP,
  OTYPE_CANIST,
  OTYPE_GREEN,
  OTYPE_VN,
  OTYPE_BERS,
  OTYPE_GOV,
  OTYPE_OMCL,
  OTYPE_TOXWC,
  OTYPE_PROBE,
  OTYPE_GR,
  OTYPE_FACTORY,
  OTYPE_TERRA,
  OTYPE_BERSCTLC,
  OTYPE_AUTOFAC,
  OTYPE_TRANSDEV,
  STYPE_MISSILE,
  OTYPE_PLANDEF,
  OTYPE_QUARRY,
  OTYPE_PLOW,
  OTYPE_DOME,
  OTYPE_WPLANT,
  OTYPE_PORT,
  OTYPE_ABM,
  OTYPE_AFV,
  OTYPE_BUNKER,
  STYPE_LANDER,
};

enum abil_t {
  ABIL_TECH,
  ABIL_CARGO,
  ABIL_HANGER,
  ABIL_DESTCAP,
  ABIL_GUNS,
  ABIL_PRIMARY,
  ABIL_SECONDARY,
  ABIL_FUELCAP,
  ABIL_MAXCREW,
  ABIL_ARMOR,
  ABIL_COST,
  ABIL_MOUNT,
  ABIL_JUMP,
  ABIL_CANLAND,
  ABIL_HASSWITCH,
  ABIL_SPEED,
  ABIL_DAMAGE,
  ABIL_BUILD,
  ABIL_CONSTRUCT,
  ABIL_MOD,
  ABIL_LASER,
  ABIL_CEW,
  ABIL_CLOAK,
  ABIL_GOD /* only diety can build these objects */,
  ABIL_PROGRAMMED,
  ABIL_PORT,
  ABIL_REPAIR,
  ABIL_MAINTAIN
};

#define NUMSTYPES (ShipType::STYPE_LANDER + 1)
#define NUMABILS (ABIL_MAINTAIN + 1)

#define SHIP_NAMESIZE 18

class Ship {
 public:
  shipnum_t number;               ///< ship knows its own number
  player_t owner;                 ///< owner of ship
  governor_t governor;            ///< subordinate that controls the ship
  char name[SHIP_NAMESIZE];       ///< name of ship (optional)
  char shipclass[SHIP_NAMESIZE];  ///< shipclass of ship - designated by player

  unsigned char race; /* race type - used when you gain alien
                         ships during revolts and whatnot - usually
                         equal to owner */
  double xpos;
  double ypos;
  double fuel;
  double mass;
  unsigned char land_x, land_y;

  shipnum_t destshipno; /* destination ship # */
  shipnum_t nextship;   /* next ship in linked list */
  shipnum_t ships;      /* ships landed on it */

  unsigned char armor;
  unsigned short size;

  unsigned short max_crew;
  resource_t max_resource;
  unsigned short max_destruct;
  unsigned short max_fuel;
  unsigned short max_speed;
  ShipType build_type;  ///< for factories - type of ship it makes
  unsigned short build_cost;

  double base_mass;
  double tech;       /* engineering technology rating */
  double complexity; /* complexity rating */

  unsigned short destruct; /* stuff it's carrying */
  resource_t resource;
  population_t popn;   /* crew */
  population_t troops; /* marines */
  unsigned short crystals;

  /* special ship functions (10 bytes) */
  union {
    struct {                  /* if the ship is a Space Mirror */
      shipnum_t shipno;       /* aimed at what ship */
      starnum_t snum;         /* aimed at what star */
      char intensity;         /* intensity of aiming */
      planetnum_t pnum;       /* aimed at what planet */
      ScopeLevel level;       /* aimed at what level */
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

  starnum_t storbits;     /* what star # orbits */
  starnum_t deststar;     /* destination star */
  planetnum_t destpnum;   /* destination planet */
  planetnum_t pnumorbits; /* # of planet if orbiting */
  ScopeLevel whatdest;    /* where going */
  ScopeLevel whatorbits;  /* where orbited */

  unsigned char damage; /* amt of damage */
  int rad;              /* radiation level */
  unsigned char retaliate;
  unsigned short target;

  ShipType type;       /* what type ship is */
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
  unsigned long primary;  /* describe primary gun system */
  unsigned long primtype;
  unsigned long secondary; /* describe secondary guns */
  unsigned long sectype;

  unsigned short hanger;     /* amount of hanger space used */
  unsigned short max_hanger; /* total hanger space */
};

class Shiplist {
 public:
  Shiplist(shipnum_t a) : first(a) {}

  class Iterator {
   public:
    Iterator(shipnum_t a);
    auto &operator*() { return elem; }
    Iterator &operator++();
    bool operator!=(const Iterator &rhs) {
      return elem.number != rhs.elem.number;
    }

   private:
    Ship elem{};
  };

  auto begin() { return Shiplist::Iterator(first); }
  auto end() { return Shiplist::Iterator(0); }

 private:
  shipnum_t first;
};

/* can takeoff & land, is mobile, etc. */
unsigned short speed_rating(const Ship &s);

bool has_switch(const Ship &d);

/* can bombard planets */
bool can_bombard(const Ship &s);

/* can navigate */
bool can_navigate(const Ship &s);

/* can aim at things. */
bool can_aim(const Ship &s);

/* macros to get ship stats */
unsigned long armor(const Ship &s);
long guns(const Ship &s);
population_t max_crew(const Ship &s);
population_t max_mil(const Ship &s);
long max_resource(const Ship &s);
int max_crystals(const Ship &s);
long max_fuel(const Ship &s);
long max_destruct(const Ship &s);
long max_speed(const Ship &s);
long shipcost(const Ship &s);
double mass(const Ship &s);
long shipsight(const Ship &s);
long retaliate(const Ship &s);
int size(const Ship &s);
int shipbody(const Ship &s);
long hanger(const Ship &s);
long repair(const Ship &s);
int getdefense(const Ship &);
bool landed(const Ship &);
bool laser_on(const Ship &);
void capture_stuff(const Ship &, GameObj &);
std::string ship_to_string(const Ship &);
double cost(const Ship &);
double getmass(const Ship &);
unsigned int ship_size(const Ship &);
double complexity(const Ship &);
bool testship(const Ship &, const player_t, const governor_t);
void kill_ship(int, Ship *);

extern shipnum_t Num_ships;
extern const long Shipdata[NUMSTYPES][NUMABILS];
extern const char Shipltrs[];
extern const char *Shipnames[];
extern int ShipVector[NUMSTYPES];

extern Ship **ships;

char *Dispshiploc_brief(Ship *);
char *Dispshiploc(Ship *);

#endif  // SHIPS_H
