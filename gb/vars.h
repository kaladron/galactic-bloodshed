// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  main bunch of variables */

#ifndef VARS_H
#define VARS_H

#include "gb/config.h"
#include "gb/tweakables.h"

/* number of movement segments (global variable) */
extern unsigned long segments;

#define MAXPLAYERS 64

struct plinfo {            /* planetary stockpiles */
  unsigned short fuel;     /* fuel for powering things */
  unsigned short destruct; /* destructive potential */
  resource_t resource;     /* resources in storage */
  population_t popn;
  population_t troops;
  unsigned short crystals;

  unsigned short prod_res; /* shows last update production */
  unsigned short prod_fuel;
  unsigned short prod_dest;
  unsigned short prod_crystals;
  money_t prod_money;
  double prod_tech;

  money_t tech_invest;
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

  long mob_points;
  double est_production; /* estimated production */
};

#define M_FUEL 0x1
#define M_DESTRUCT 0x2
#define M_RESOURCES 0x4
#define M_CRYSTALS 0x8
#define Fuel(x) ((x)&M_FUEL)
#define Destruct(x) ((x)&M_DESTRUCT)
#define Resources(x) ((x)&M_RESOURCES)
#define Crystals(x) ((x)&M_CRYSTALS)

class Race;

class Planet {
 public:
  Planet() = default;
  Planet(Planet &) = delete;
  Planet &operator=(const Planet &) = delete;
  Planet(Planet &&) = default;
  Planet &operator=(Planet &&) = default;

  double gravity() const;
  double compatibility(const Race &) const;
  ap_t get_points() const;

  double xpos, ypos;        /* x,y relative to orbit */
  shipnum_t ships;          /* first ship in orbit (to be changed) */
  unsigned char Maxx, Maxy; /* size of map */

  struct plinfo info[MAXPLAYERS]; /* player info */
  int conditions[TOXIC + 1];      /* atmospheric conditions for terraforming */

  population_t popn;
  population_t troops;
  population_t maxpopn; /* maximum population */
  resource_t total_resources;

  player_t slaved_to;
  PlanetType type;         /* what type planet is */
  unsigned char expltimer; /* timer for explorations */

  unsigned char explored;

  planetnum_t planet_id;
};

class SectorMap {
 public:
  SectorMap(const Planet &planet) : maxx_(planet.Maxx), maxy_(planet.Maxy) {
    vec_.reserve(planet.Maxx * planet.Maxy);
  }

  //! Add an empty sector for every potential space.  Used for initialization.
  SectorMap(const Planet &planet, bool)
      : maxx_(planet.Maxx),
        maxy_(planet.Maxy),
        vec_(planet.Maxx * planet.Maxy) {}

  // TODO(jeffbailey): Should wrap this in a subclass so the underlying
  // vector isn't exposed to callers.
  auto begin() { return vec_.begin(); }
  auto end() { return vec_.end(); }

  Sector &get(const int x, const int y) {
    return vec_.at(static_cast<size_t>(x + (y * maxx_)));
  }
  void put(Sector &&s) { vec_.emplace_back(std::move(s)); }
  int get_maxx() { return maxx_; }
  int get_maxy() { return maxy_; }
  Sector &get_random();
  // TODO(jeffbailey): Don't expose the underlying vector.
  std::vector<std::reference_wrapper<Sector>>
  shuffle();  /// Randomizes the order of the SectorMap.

  SectorMap(SectorMap &) = delete;
  void operator=(const SectorMap &) = delete;
  SectorMap(SectorMap &&) = default;
  SectorMap &operator=(SectorMap &&) = default;

 private:
  SectorMap(const int maxx, const int maxy) : maxx_(maxx), maxy_(maxy) {}
  int maxx_;
  int maxy_;
  std::vector<Sector> vec_;
};

struct Star {
  unsigned short ships;            /* 1st ship in orbit */
  char name[NAMESIZE];             /* name of star */
  governor_t governor[MAXPLAYERS]; /* which subordinate maintains the system */
  ap_t AP[MAXPLAYERS];             /* action pts alotted */
  uint64_t explored;               /* who's been here 64 bits*/
  uint64_t inhabited;              /* who lives here now 64 bits*/
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
  unsigned short numstars; /* # of stars */
  unsigned short ships;    /* 1st ship in orbit */
  ap_t AP[MAXPLAYERS];     /* Action pts for each player */
  unsigned short VN_hitlist[MAXPLAYERS];
  /* # of ships destroyed by each player */
  char VN_index1[MAXPLAYERS]; /* negative value is used */
  char VN_index2[MAXPLAYERS]; /* VN's record of destroyed ships
                                        systems where they bought it */
  unsigned long dummy[2];
};

extern struct stardata Sdata;

extern unsigned char Nuked[MAXPLAYERS];
extern unsigned long StarsInhab[NUMSTARS];
extern unsigned long StarsExpl[NUMSTARS];
extern std::vector<Star> stars;
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

extern std::array<std::array<std::unique_ptr<Planet>, MAXPLANETS>, NUMSTARS>
    planets;
extern unsigned char ground_assaults[MAXPLAYERS][MAXPLAYERS][NUMSTARS];
extern uint64_t inhabited[NUMSTARS];
extern double Compat[MAXPLAYERS];
extern player_t Num_races;
extern unsigned long Num_commods;
extern planetnum_t Planet_count;
extern unsigned long newslength[4];

#endif  // VARS_H
