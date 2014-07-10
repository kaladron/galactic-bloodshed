// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/*  main bunch of variables */

#ifndef VARS_H
#define VARS_H

#include "files.h"
#include "tweakables.h"
#include "config.h"
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>

unsigned long segments; /* number of movement segments (global variable) */

#define MAX_ROUTES                                                             \
  4 /* Shipping routes - DON'T change this unless you know                     \
       what you are doing */

#define LEVEL_UNIV 0
#define LEVEL_STAR 1
#define LEVEL_PLAN 2
#define LEVEL_SHIP 3

#define MAXPLAYERS 64
#define MAXSTRLEN 2047
#define HUGESTRLEN (2 * MAXSTRLEN + 1)
#define SMALLSTR 31

#define ANN 0
#define BROADCAST 1
#define SHOUT 2
#define THINK 3

#define LEADER 0
#define GENERAL 1
#define CAPTIAN 2
#define PRIVATE 3
#define NOVICE 4
#define ERRORLOG 0
#define NOERRNO 0
typedef char hugestr[HUGESTRLEN];

long random();

typedef struct sector sectortype;
typedef struct planet planettype;
typedef struct star startype;
typedef struct commod commodtype;

struct plinfo {            /* planetary stockpiles */
  unsigned short fuel;     /* fuel for powering things */
  unsigned short destruct; /* destructive potential */
  unsigned short resource; /* resources in storage */
  unsigned long popn;
  unsigned long troops;
  unsigned short crystals;

  unsigned short prod_res; /* shows last update production */
  unsigned short prod_fuel;
  unsigned short prod_dest;
  unsigned short prod_crystals;
  unsigned long prod_money;
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
#define Fuel(x) ((x) & M_FUEL)
#define Destruct(x) ((x) & M_DESTRUCT)
#define Resources(x) ((x) & M_RESOURCES)
#define Crystals(x) ((x) & M_CRYSTALS)

struct commod {
  char owner;
  char governor;
  char type;
  unsigned short amount;
  unsigned char dummy;
  unsigned char deliver; /* whether the lot is ready for shipping or not */
  unsigned long bid;
  unsigned char bidder;
  unsigned char bidder_gov;
  unsigned char star_from; /* where the stuff originated from */
  unsigned char planet_from;
  unsigned char star_to; /* where it goes to */
  unsigned char planet_to;
};

struct sector {
  unsigned char eff;          /* efficiency (0-100) */
  unsigned char fert;         /* max popn is proportional to this */
  unsigned char mobilization; /* percent popn is mobilized for war */
  unsigned char crystals;
  unsigned short resource;

  unsigned short popn;
  unsigned short troops; /* troops (additional combat value) */

  unsigned char owner; /* owner of place */
  unsigned char race;  /* race type occupying sector
                      (usually==owner) - makes things more
                      realistic when alien races revolt and
                      you gain control of them! */
  unsigned char type;      /* underlying sector geology */
  unsigned char condition; /* environmental effects */
  unsigned long dummy2;
};

struct planet {
  int sectormappos; /* file posn for sector map */

  double xpos, ypos;        /* x,y relative to orbit */
  unsigned short ships;     /* first ship in orbit (to be changed) */
  unsigned char Maxx, Maxy; /* size of map */

  struct plinfo info[MAXPLAYERS]; /* player info */
  short conditions[TOXIC + 1];    /* atmospheric conditions for terraforming */

  unsigned long popn;
  unsigned long troops;
  unsigned long maxpopn; /* maximum population */
  unsigned long total_resources;

  unsigned char slaved_to;
  unsigned char type;      /* what type planet is */
  unsigned char expltimer; /* timer for explorations */

  unsigned char explored;

  unsigned long dummy[2];
};

struct star {
  unsigned short ships;         /* 1st ship in orbit */
  char name[NAMESIZE];          /* name of star */
  char governor[MAXPLAYERS];    /* which subordinate maintains the system */
  unsigned char AP[MAXPLAYERS]; /* action pts alotted */
  unsigned long explored[2];    /* who's been here 64 bits*/
  unsigned long inhabited[2];   /* who lives here now 64 bits*/
  double xpos, ypos;

  unsigned char numplanets;            /* # of planets in star system */
  char pnames[MAXPLANETS][NAMESIZE];   /* names of planets */
  unsigned long planetpos[MAXPLANETS]; /* file posns of planets */

  unsigned char stability;   /* how close to nova it is */
  unsigned char nova_stage;  /* stage of nova */
  unsigned char temperature; /* factor which expresses how hot the star is*/
  double gravity;            /* attraction of star in "Standards". */

  long dummy[2]; /* dummy bits for development */
};

/* this data will all be read at once */
struct stardata {
  unsigned short numstars;      /* # of stars */
  unsigned short ships;         /* 1st ship in orbit */
  unsigned char AP[MAXPLAYERS]; /* Action pts for each player */
  unsigned short VN_hitlist[MAXPLAYERS];
  /* # of ships destroyed by each player */
  unsigned char VN_index1[MAXPLAYERS]; /* negative value is used */
  unsigned char VN_index2[MAXPLAYERS]; /* VN's record of destroyed ships
                                        systems where they bought it */
  unsigned long dummy[2];
};

EXTERN struct stardata Sdata;

struct directory {
  unsigned char level;                /* what directory level */
  unsigned char snum;                 /* what star system obj # (level=0) */
  unsigned char pnum;                 /* number of planet */
  unsigned short shipno;              /* # of ship */
  char prompt[3 * NAMESIZE + 5];      /* just to be safe */
  double lastx[2], lasty[2], zoom[2]; /* last coords for zoom */
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

EXTERN struct directory Dir[MAXPLAYERS][MAXGOVERNORS + 1];

EXTERN sectortype Smap[(MAX_X + 1) * (MAX_Y + 1) + 1];

EXTERN unsigned char Nuked[MAXPLAYERS];
EXTERN unsigned long StarsInhab[NUMSTARS];
EXTERN unsigned long StarsExpl[NUMSTARS];
EXTERN startype *Stars[NUMSTARS];
EXTERN unsigned short Sdatanumships[MAXPLAYERS];
EXTERN unsigned long Sdatapopns[MAXPLAYERS];
EXTERN unsigned short starnumships[NUMSTARS][MAXPLAYERS];
EXTERN unsigned long starpopns[NUMSTARS][MAXPLAYERS];

EXTERN unsigned long tot_resdep, prod_eff, prod_res[MAXPLAYERS];
EXTERN unsigned long prod_fuel[MAXPLAYERS], prod_destruct[MAXPLAYERS];
EXTERN unsigned long prod_crystals[MAXPLAYERS], prod_money[MAXPLAYERS];
EXTERN unsigned long tot_captured, prod_mob;
EXTERN unsigned long avg_mob[MAXPLAYERS];
EXTERN unsigned char sects_gained[MAXPLAYERS], sects_lost[MAXPLAYERS];
EXTERN unsigned char Claims;
EXTERN unsigned char adr;
EXTERN char junk[2][256];

EXTERN planettype *planets[NUMSTARS][MAXPLANETS];
EXTERN unsigned char ground_assaults[MAXPLAYERS][MAXPLAYERS][NUMSTARS];
EXTERN unsigned long inhabited[NUMSTARS][2];
EXTERN double Compat[MAXPLAYERS];
EXTERN unsigned long Num_races, Num_ships, Num_commods;
EXTERN unsigned long Planet_count;
EXTERN unsigned long newslength[4];
EXTERN char args[MAXARGS][COMMANDSIZE];
EXTERN int argn;

/* bit routines stolen from UNIX <sys/param.h> */
#define setbit(a, i) ((a)[(i) / 32] |= ((i) < 32 ? 1 << (i) : 1 << ((i) - 32)))
#define clrbit(a, i) ((a)[(i) / 32] &= ~((i) < 32 ? 1 << (i) : 1 << ((i) - 32)))
#define isset(a, i) ((a)[(i) / 32] & ((i) < 32 ? 1 << (i) : 1 << ((i) - 32)))
#define isclr(a, i) (!isset((a), (i)))

#ifdef DEBUG /* for debugging option */
#define malloc(s) DEBUGmalloc(s, __FILE__, __LINE__)
#define free(s) DEBUGfree(s)
#define realloc(p, s) DEBUGrealloc(p, s, __FILE__, __LINE__)

#define getrace(a, b, c) DEBUGgetrace(a, b, c, __FILE__, __LINE__)
#define getstar(a, b, c) DEBUGgetstar(a, b, c, __FILE__, __LINE__)
#define getplanet(a, b, c) DEBUGgetplanet(a, b, c, __FILE__, __LINE__)
#define getship(a, b, c) DEBUGgetship(a, b, c, __FILE__, __LINE__)
#define getcommod(a, b, c) DEBUGgetcommod(a, b, c, __FILE__, __LINE__)
#endif

#define success(x) (int_rand(1, 100) <= (x))

#endif // VARS_H
