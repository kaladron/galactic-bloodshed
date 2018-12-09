// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  main bunch of variables */

#ifndef VARS_H
#define VARS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include "config.h"
#include "files.h"
#include "tweakables.h"

/* number of movement segments (global variable) */
extern unsigned long segments;

/* Shipping routes - DON'T change this unless you know what you are doing */
const int MAX_ROUTES = 4;

enum ScopeLevel { LEVEL_UNIV, LEVEL_STAR, LEVEL_PLAN, LEVEL_SHIP };

typedef uint64_t shipnum_t;
typedef uint8_t starnum_t;
typedef uint8_t planetnum_t;
typedef uint8_t player_t;
typedef uint8_t governor_t;
typedef uint64_t commodnum_t;

typedef int64_t money_t;
typedef uint64_t population_t;

typedef std::vector<std::string> command_t;

#define MAXPLAYERS 64
#define MAXSTRLEN 2047
#define HUGESTRLEN (2 * MAXSTRLEN + 1)

typedef char hugestr[HUGESTRLEN];

typedef class Planet planettype;
typedef struct star startype;
typedef struct commod commodtype;

struct plinfo {            /* planetary stockpiles */
  unsigned short fuel;     /* fuel for powering things */
  unsigned short destruct; /* destructive potential */
  unsigned short resource; /* resources in storage */
  population_t popn;
  population_t troops;
  unsigned short crystals;

  unsigned short prod_res; /* shows last update production */
  unsigned short prod_fuel;
  unsigned short prod_dest;
  unsigned short prod_crystals;
  money_t prod_money;
  double prod_tech;

  unsigned short tech_invest;
  unsigned short numsectsowned;

  unsigned char comread;    /* combat readiness (mobilization)*/
  unsigned char mob_set;    /* mobilization target */
  unsigned char tox_thresh; /* min to build a waste can */

  unsigned char explored;
  unsigned char autorep;
  unsigned char tax;    /* tax rate */
  unsigned char newtax; /* new tax rate (after update) */
  unsigned char guns;   /* number of planet guns (mob/5) */

  /* merchant shipping parameters */
  struct {
    unsigned char set;         /* does the planet have orders? */
    unsigned char dest_star;   /* star that ship has to go to next */
    unsigned char dest_planet; /* planet destination */
    unsigned char load;        /* bit-field commodities to be loaded there */
    unsigned char unload;      /* unloaded commodities */
    unsigned char x, y;        /* location that ship has to land on */
  } route[MAX_ROUTES];         /* i am allowing up to four routes per planet */

  unsigned long mob_points;
  double est_production; /* estimated production */
  unsigned long dummy[3];
};

#define M_FUEL 0x1
#define M_DESTRUCT 0x2
#define M_RESOURCES 0x4
#define M_CRYSTALS 0x8
#define Fuel(x) ((x)&M_FUEL)
#define Destruct(x) ((x)&M_DESTRUCT)
#define Resources(x) ((x)&M_RESOURCES)
#define Crystals(x) ((x)&M_CRYSTALS)

struct commod {
  player_t owner;
  governor_t governor;
  uint8_t type;
  uint64_t amount;
  unsigned char dummy;
  unsigned char deliver; /* whether the lot is ready for shipping or not */
  money_t bid;
  player_t bidder;
  governor_t bidder_gov;
  starnum_t star_from; /* where the stuff originated from */
  planetnum_t planet_from;
  starnum_t star_to; /* where it goes to */
  planetnum_t planet_to;
};

class GameObj {
 public:
  player_t player;
  governor_t governor;
  bool god;
  double lastx[2] = {0.0, 0.0};
  double lasty[2] = {0.0, 0.0};
  double zoom[2] = {1.0, 0.5}; /* last coords for zoom */
  GameObj() = default;
  GameObj(const GameObj &) = delete;
  GameObj &operator=(const GameObj &) = delete;
};

class Planet {
 public:
  double xpos, ypos;        /* x,y relative to orbit */
  shipnum_t ships;          /* first ship in orbit (to be changed) */
  unsigned char Maxx, Maxy; /* size of map */

  struct plinfo info[MAXPLAYERS]; /* player info */
  int conditions[TOXIC + 1];      /* atmospheric conditions for terraforming */

  population_t popn;
  population_t troops;
  population_t maxpopn; /* maximum population */
  unsigned long total_resources;

  player_t slaved_to;
  PlanetType type;         /* what type planet is */
  unsigned char expltimer; /* timer for explorations */

  unsigned char explored;

  planetnum_t planet_id;
};

class sector {
 public:
  sector(int eff_, int fert_, int mobilization_, int crystals_, int resource_,
         int popn_, int troops_, int owner_, int race_, int type_,
         int condition_)
      : eff(eff_),
        fert(fert_),
        mobilization(mobilization_),
        crystals(crystals_),
        resource(resource_),
        popn(popn_),
        troops(troops_),
        owner(owner_),
        race(race_),
        type(type_),
        condition(condition_) {}
  sector()
      : eff(0),
        fert(0),
        mobilization(0),
        crystals(0),
        resource(0),
        popn(0),
        troops(0),
        owner(0),
        race(0),
        type(0),
        condition(0) {}
  unsigned int eff;          /* efficiency (0-100) */
  unsigned int fert;         /* max popn is proportional to this */
  unsigned int mobilization; /* percent popn is mobilized for war */
  unsigned int crystals;
  unsigned int resource;

  population_t popn;
  population_t troops; /* troops (additional combat value) */

  player_t owner;         /* owner of place */
  player_t race;          /* race type occupying sector
                 (usually==owner) - makes things more
                 realistic when alien races revolt and
                 you gain control of them! */
  unsigned int type;      /* underlying sector geology */
  unsigned int condition; /* environmental effects */
  sector(sector &) = delete;
  void operator=(const sector &) = delete;
  sector(sector &&) = default;
  sector &operator=(sector &&) = default;
  friend std::ostream &operator<<(std::ostream &, const sector &);
};

class sector_map {
 public:
  sector_map(const Planet &planet) : maxx_(planet.Maxx), maxy_(planet.Maxy) {
    vec_.reserve(planet.Maxx * planet.Maxy);
  }

  //! Add an empty sector for every potential space.  Used for initialization.
  sector_map(const Planet &planet, bool)
      : maxx_(planet.Maxx),
        maxy_(planet.Maxy),
        vec_(planet.Maxx * planet.Maxy) {}

  sector &get(const int x, const int y) { return vec_.at((x) + (y)*maxx_); }
  void put(sector &&s) { vec_.emplace_back(std::move(s)); }
  int get_maxx() { return maxx_; }
  int get_maxy() { return maxy_; }
  sector &get_random();

  sector_map(sector_map &) = delete;
  void operator=(const sector_map &) = delete;
  sector_map(sector_map &&) = default;
  sector_map &operator=(sector_map &&) = default;

 private:
  sector_map(const int maxx, const int maxy) : maxx_(maxx), maxy_(maxy) {}
  const int maxx_;
  const int maxy_;
  std::vector<sector> vec_;
};

struct star {
  unsigned short ships;            /* 1st ship in orbit */
  char name[NAMESIZE];             /* name of star */
  governor_t governor[MAXPLAYERS]; /* which subordinate maintains the system */
  unsigned int AP[MAXPLAYERS];     /* action pts alotted */
  unsigned long explored[2];       /* who's been here 64 bits*/
  unsigned long inhabited[2];      /* who lives here now 64 bits*/
  double xpos, ypos;

  unsigned char numplanets;            /* # of planets in star system */
  char pnames[MAXPLANETS][NAMESIZE];   /* names of planets */
  unsigned long planetpos[MAXPLANETS]; /* file posns of planets */

  unsigned char stability;   /* how close to nova it is */
  unsigned char nova_stage;  /* stage of nova */
  unsigned char temperature; /* factor which expresses how hot the star is*/
  double gravity;            /* attraction of star in "Standards". */

  starnum_t star_id;
  long dummy[1]; /* dummy bits for development */
};

/* this data will all be read at once */
struct stardata {
  unsigned short numstars;     /* # of stars */
  unsigned short ships;        /* 1st ship in orbit */
  unsigned int AP[MAXPLAYERS]; /* Action pts for each player */
  unsigned short VN_hitlist[MAXPLAYERS];
  /* # of ships destroyed by each player */
  char VN_index1[MAXPLAYERS]; /* negative value is used */
  char VN_index2[MAXPLAYERS]; /* VN's record of destroyed ships
                                        systems where they bought it */
  unsigned long dummy[2];
};

extern struct stardata Sdata;

struct directory {
  ScopeLevel level; /* what directory level */
  starnum_t snum;   /* what star system obj # (level=0) */
  planetnum_t pnum; /* number of planet */
  shipnum_t shipno; /* # of ship */
};

struct vic {
  unsigned char racenum;
  char name[RNAMESIZE];
  unsigned long no_count;
  char sleep;
  double tech;
  int Thing;
  int IQ;
  long rawscore;
  long login;
};

extern struct directory Dir[MAXPLAYERS][MAXGOVERNORS + 1];

extern unsigned char Nuked[MAXPLAYERS];
extern unsigned long StarsInhab[NUMSTARS];
extern unsigned long StarsExpl[NUMSTARS];
extern startype *Stars[NUMSTARS];
extern unsigned short Sdatanumships[MAXPLAYERS];
extern unsigned long Sdatapopns[MAXPLAYERS];
extern unsigned short starnumships[NUMSTARS][MAXPLAYERS];
extern unsigned long starpopns[NUMSTARS][MAXPLAYERS];

extern unsigned long tot_resdep, prod_eff, prod_res[MAXPLAYERS];
extern unsigned long prod_fuel[MAXPLAYERS], prod_destruct[MAXPLAYERS];
extern unsigned long prod_crystals[MAXPLAYERS], prod_money[MAXPLAYERS];
extern unsigned long tot_captured, prod_mob;
extern unsigned long avg_mob[MAXPLAYERS];
extern unsigned char sects_gained[MAXPLAYERS], sects_lost[MAXPLAYERS];
extern unsigned char Claims;

extern planettype *planets[NUMSTARS][MAXPLANETS];
extern unsigned char ground_assaults[MAXPLAYERS][MAXPLAYERS][NUMSTARS];
extern unsigned long inhabited[NUMSTARS][2];
extern double Compat[MAXPLAYERS];
extern player_t Num_races;
extern unsigned long Num_commods;
extern planetnum_t Planet_count;
extern unsigned long newslength[4];
extern char args[MAXARGS][COMMANDSIZE];
extern int argn;

/* bit routines stolen from UNIX <sys/param.h> */
#define setbit(a, i) ((a)[(i) / 32] |= ((i) < 32 ? 1 << (i) : 1 << ((i)-32)))
#define clrbit(a, i) ((a)[(i) / 32] &= ~((i) < 32 ? 1 << (i) : 1 << ((i)-32)))
#define isset(a, i) ((a)[(i) / 32] & ((i) < 32 ? 1 << (i) : 1 << ((i)-32)))
#define isclr(a, i) (!isset((a), (i)))

#endif  // VARS_H
