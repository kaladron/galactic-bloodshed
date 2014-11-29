// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* enrol.c -- initializes to owner one sector and planet. */

#include <curses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "GB_server.h"
#include "buffers.h"
#include "build.h"
#include "files_shl.h"
#include "map.h"
#include "max.h"
#include "perm.h"
#include "races.h"
#include "rand.h"
#include "shipdata.h"
#include "ships.h"
#include "tweakables.h"
#include "vars.h"

#include "globals.h"

struct stype {
  char here;
  char x, y;
  int count;
};

#define RACIAL_TYPES 10

// TODO(jeffbailey): Copied from map.c, but they've diverged
static char desshow(planettype *p, int x, int y, sector_map &);

/* racial types (10 racial types ) */
static int Thing[RACIAL_TYPES] = {1, 1, 1, 0, 0, 0, 0, 0, 0, 0};

static double db_Mass[RACIAL_TYPES] = {.1,   .15,  .2,   .125, .125,
                                       .125, .125, .125, .125, .125};
static double db_Birthrate[RACIAL_TYPES] = {0.9, 0.85, 0.8, 0.5,  0.55,
                                            0.6, 0.65, 0.7, 0.75, 0.8};
static int db_Fighters[RACIAL_TYPES] = {9, 10, 11, 2, 3, 4, 5, 6, 7, 8};
static int db_Intelligence[RACIAL_TYPES] = {0,   0,   0,   190, 180,
                                            170, 160, 150, 140, 130};

static double db_Adventurism[RACIAL_TYPES] = {0.89, 0.89, 0.89, .6,  .65,
                                              .7,   .7,   .75,  .75, .8};

static int Min_Sexes[RACIAL_TYPES] = {1, 1, 1, 2, 2, 2, 2, 2, 2, 2};
static int Max_Sexes[RACIAL_TYPES] = {1, 1, 1, 2, 2, 4, 4, 4, 4, 4};
static double db_Metabolism[RACIAL_TYPES] = {3.0,  2.7,  2.4, 1.0,  1.15,
                                             1.30, 1.45, 1.6, 1.75, 1.9};

#define RMass(x) (db_Mass[(x)] + .001 * (double)int_rand(-25, 25))
#define Birthrate(x) (db_Birthrate[(x)] + .01 * (double)int_rand(-10, 10))
#define Fighters(x) (db_Fighters[(x)] + int_rand(-1, 1))
#define Intelligence(x) (db_Intelligence[(x)] + int_rand(-10, 10))
#define Adventurism(x) (db_Adventurism[(x)] + 0.01 * (double)int_rand(-10, 10))
#define Sexes(x) \
  (int_rand(Min_Sexes[(x)], int_rand(Min_Sexes[(x)], Max_Sexes[(x)])))
#define Metabolism(x) (db_Metabolism[(x)] + .01 * (double)int_rand(-15, 15))

int main() {
  int x, y;
  int pnum, star = 0, found = 0, check, vacant, count, i, j, Playernum;
  int ppref = -1;
  sigset_t mask, block;
  int s, idx, k;
#define STRSIZE 100
  char str[STRSIZE], c;
  struct stype secttypes[WASTED + 1] = {};
  planettype *planet;
  unsigned char not_found[TYPE_DESERT + 1];
  startype *star_arena;

  open_data_files();

  srandom(getpid());

  if ((Playernum = Numraces() + 1) >= MAXPLAYERS) {
    printf("There are already %d players; No more allowed.\n", MAXPLAYERS - 1);
    exit(-1);
  }

  printf("Enter racial type to be created (1-%d):", RACIAL_TYPES);
  scanf("%d", &idx);
  getchr();

  if (idx <= 0 || idx > RACIAL_TYPES) {
    printf("Bad racial index.\n");
    exit(1);
  }
  idx = idx - 1;

  getsdata(&Sdata);

  star_arena = (startype *)malloc(Sdata.numstars * sizeof(startype));
  for (s = 0; s < Sdata.numstars; s++) {
    Stars[s] = &star_arena[s];
    getstar(&(Stars[s]), s);
  }
  printf("There is still space for player %d.\n", Playernum);

  bzero((char *)not_found, sizeof(not_found));
  do {
    printf(
        "\nLive on what type planet:\n     (e)arth, (g)asgiant, (m)ars, "
        "(i)ce, (w)ater, (d)esert, (f)orest? ");
    c = getchr();
    getchr();

    switch (c) {
      case 'w':
        ppref = TYPE_WATER;
        break;
      case 'e':
        ppref = TYPE_EARTH;
        break;
      case 'm':
        ppref = TYPE_MARS;
        break;
      case 'g':
        ppref = TYPE_GASGIANT;
        break;
      case 'i':
        ppref = TYPE_ICEBALL;
        break;
      case 'd':
        ppref = TYPE_DESERT;
        break;
      case 'f':
        ppref = TYPE_FOREST;
        break;
      default:
        printf("Oh well.\n");
        exit(-1);
    }

    printf("Looking for type %d planet...\n", ppref);

    /* find first planet of right type */
    count = 0;
    found = 0;

    for (star = 0; star < Sdata.numstars && !found && count < 100;) {
      check = 1;
      /* skip over inhabited stars - or stars with just one planet! */
      if (Stars[star]->inhabited[0] + Stars[star]->inhabited[1] ||
          Stars[star]->numplanets < 2)
        check = 0;

      /* look for uninhabited planets */
      if (check) {
        pnum = 0;
        while (!found && pnum < Stars[star]->numplanets) {
          getplanet(&planet, star, pnum);

          if (planet->type == ppref && Stars[star]->numplanets != 1) {
            vacant = 1;
            for (i = 1; i <= Playernum; i++)
              if (planet->info[i - 1].numsectsowned) vacant = 0;
            if (vacant && planet->conditions[RTEMP] >= -50 &&
                planet->conditions[RTEMP] <= 50) {
              found = 1;
            }
          }
          if (!found) {
            free(planet);
            pnum++;
          }
        }
      }

      if (!found) {
        count++;
        star = int_rand(0, Sdata.numstars - 1);
      }
    }

    if (!found) {
      printf("planet type not found in any free systems.\n");
      not_found[ppref] = 1;
      for (found = 1, i = TYPE_EARTH; i <= TYPE_DESERT; i++)
        found &= not_found[i];
      if (found) {
        printf("Looks like there aren't any free planets left.  bye..\n");
        endwin();
        exit(-1);
      } else
        printf("  Try a different one...\n");
      found = 0;
    }

  } while (!found);

  auto Race = new race;
  bzero(Race, sizeof(Race));

  printf("\n\tDeity/Guest/Normal (d/g/n) ?");
  c = getchr();
  getchr();

  Race->God = (c == 'd');
  Race->Guest = (c == 'g');
  strcpy(Race->name, "Unknown");

  for (i = 0; i <= MAXGOVERNORS; i++) {
    Race->governor[0].money = 0;
  }
  Race->governor[0].homelevel = Race->governor[0].deflevel = LEVEL_PLAN;
  Race->governor[0].homesystem = Race->governor[0].defsystem = star;
  Race->governor[0].homeplanetnum = Race->governor[0].defplanetnum = pnum;
  /* display options */
  Race->governor[0].toggle.highlight = Playernum;
  Race->governor[0].toggle.inverse = 1;
  Race->governor[0].toggle.color = 0;
  Race->governor[0].active = 1;
  printf("Enter the password for this race:");
  scanf("%s", Race->password);
  getchr();
  printf("Enter the password for this leader:");
  scanf("%s", Race->governor[0].password);
  getchr();

  /* make conditions preferred by your people set to (more or less)
     those of the planet : higher the concentration of gas, the higher
     percentage difference between planet and race (commented out) */
  for (j = 0; j <= OTHER; j++) Race->conditions[j] = planet->conditions[j];
  /*+ int_rand( round_rand(-planet->conditions[j]*2.0),
   * round_rand(planet->conditions[j]*2.0) )*/

  for (i = 0; i < MAXPLAYERS; i++) {
    /* messages from autoreport, player #1 are decodable */
    if ((i == (Playernum - 1) || Playernum == 1) || Race->God)
      Race->translate[i] = 100; /* you can talk to own race */
    else
      Race->translate[i] = 1;
  }

  /* assign racial characteristics */
  for (i = 0; i < NUM_DISCOVERIES; i++) Race->discoveries[i] = 0;
  Race->tech = 0.0;
  Race->morale = 0;
  Race->turn = 0;
  Race->allied[0] = Race->allied[1] = 0;
  Race->atwar[0] = Race->atwar[1] = 0;
  do {
    Race->mass = RMass(idx);
    Race->birthrate = Birthrate(idx);
    Race->fighters = Fighters(idx);
    if (Thing[idx]) {
      Race->IQ = 0;
      Race->Metamorph = Race->absorb = Race->collective_iq = Race->pods = 1;
    } else {
      Race->IQ = Intelligence(idx);
      Race->Metamorph = Race->absorb = Race->collective_iq = Race->pods = 0;
    }
    Race->adventurism = Adventurism(idx);
    Race->number_sexes = Sexes(idx);
    Race->metabolism = Metabolism(idx);

    printf("%s\n", Race->Metamorph ? "METAMORPHIC" : "");
    printf("       Birthrate: %.3f\n", Race->birthrate);
    printf("Fighting ability: %d\n", Race->fighters);
    printf("              IQ: %d\n", Race->IQ);
    printf("      Metabolism: %.2f\n", Race->metabolism);
    printf("     Adventurism: %.2f\n", Race->adventurism);
    printf("            Mass: %.2f\n", Race->mass);
    printf(" Number of sexes: %d (min req'd for colonization)\n",
           Race->number_sexes);

    printf("\n\nLook OK(y/n)\?");
    if (fgets(str, STRSIZE, stdin) == NULL) exit(1);
  } while (str[0] != 'y');

  auto smap = getsmap(*planet);

  printf(
      "\nChoose a primary sector preference. This race will prefer to "
      "live\non this type of sector.\n");

  PermuteSects(planet);
  Getxysect(planet, 0, 0, 1);
  while (Getxysect(planet, &x, &y, 0)) {
    secttypes[smap.get(x, y).condition].count++;
    if (!secttypes[smap.get(x, y).condition].here) {
      secttypes[smap.get(x, y).condition].here = 1;
      secttypes[smap.get(x, y).condition].x = x;
      secttypes[smap.get(x, y).condition].y = y;
    }
  }
  planet->explored = 1;
  for (i = SEA; i <= WASTED; i++)
    if (secttypes[i].here) {
      printf("(%2d): %c (%d, %d) (%s, %d sectors)\n", i,
             desshow(planet, secttypes[i].x, secttypes[i].y, smap),
             secttypes[i].x, secttypes[i].y, Desnames[i], secttypes[i].count);
    }
  planet->explored = 0;

  found = 0;
  do {
    printf("\nchoice (enter the number): ");
    scanf("%d", &i);
    getchr();
    if (i < SEA || i > WASTED || !secttypes[i].here) {
      printf("There are none of that type here..\n");
    } else
      found = 1;
  } while (!found);

  auto &sect = smap.get(secttypes[i].x, secttypes[i].y);
  Race->likesbest = i;
  Race->likes[i] = 1.0;
  Race->likes[PLATED] = 1.0;
  Race->likes[WASTED] = 0.0;
  printf("\nEnter compatibilities of other sectors -\n");
  for (j = SEA; j < PLATED; j++)
    if (i != j) {
      printf("%6s (%3d sectors) :", Desnames[j], secttypes[j].count);
      scanf("%d", &k);
      Race->likes[j] = (double)k / 100.0;
    }
  printf("Numraces = %d\n", Numraces());
  Playernum = Race->Playernum = Numraces() + 1;

  sigemptyset(&block);
  sigaddset(&block, SIGHUP);
  sigaddset(&block, SIGTERM);
  sigaddset(&block, SIGINT);
  sigaddset(&block, SIGQUIT);
  sigaddset(&block, SIGSTOP);
  sigaddset(&block, SIGTSTP);
  sigprocmask(SIG_BLOCK, &block, &mask);
  /* build a capital ship to run the government */
  {
    shiptype s;
    int shipno;

    bzero(&s, sizeof(s));
    shipno = Numships() + 1;
    printf("Creating government ship %d...\n", shipno);
    Race->Gov_ship = shipno;
    planet->ships = shipno;
    s.nextship = 0;

    s.type = OTYPE_GOV;
    s.xpos = Stars[star]->xpos + planet->xpos;
    s.ypos = Stars[star]->ypos + planet->ypos;
    s.land_x = (char)secttypes[i].x;
    s.land_y = (char)secttypes[i].y;

    s.speed = 0;
    s.owner = Playernum;
    s.race = Playernum;
    s.governor = 0;

    s.tech = 100.0;

    s.build_type = OTYPE_GOV;
    s.armor = Shipdata[OTYPE_GOV][ABIL_ARMOR];
    s.guns = PRIMARY;
    s.primary = Shipdata[OTYPE_GOV][ABIL_GUNS];
    s.primtype = Shipdata[OTYPE_GOV][ABIL_PRIMARY];
    s.secondary = Shipdata[OTYPE_GOV][ABIL_GUNS];
    s.sectype = Shipdata[OTYPE_GOV][ABIL_SECONDARY];
    s.max_crew = Shipdata[OTYPE_GOV][ABIL_MAXCREW];
    s.max_destruct = Shipdata[OTYPE_GOV][ABIL_DESTCAP];
    s.max_resource = Shipdata[OTYPE_GOV][ABIL_CARGO];
    s.max_fuel = Shipdata[OTYPE_GOV][ABIL_FUELCAP];
    s.max_speed = Shipdata[OTYPE_GOV][ABIL_SPEED];
    s.build_cost = Shipdata[OTYPE_GOV][ABIL_COST];
    s.size = 100;
    s.base_mass = 100.0;
    sprintf(s.shipclass, "Standard");

    s.fuel = 0.0;
    s.popn = Shipdata[s.type][ABIL_MAXCREW];
    s.troops = 0;
    s.mass = s.base_mass + Shipdata[s.type][ABIL_MAXCREW] * Race->mass;
    s.destruct = s.resource = 0;

    s.alive = 1;
    s.active = 1;
    s.protect.self = 1;

    s.docked = 1;
    /* docked on the planet */
    s.whatorbits = LEVEL_PLAN;
    s.whatdest = LEVEL_PLAN;
    s.deststar = star;
    s.destpnum = pnum;
    s.storbits = star;
    s.pnumorbits = pnum;
    s.rad = 0;
    s.damage = 0; /*Shipdata[s.type][ABIL_DAMAGE];*/
    /* (first capital is 100% efficient */
    s.retaliate = 0;

    s.ships = 0;

    s.on = 1;

    s.name[0] = '\0';
    s.number = shipno;
    printf("Created on sector %d,%d on /%s/%s\n", s.land_x, s.land_y,
           Stars[s.storbits]->name, Stars[s.storbits]->pnames[s.pnumorbits]);
    putship(&s);
  }

  for (j = 0; j < MAXPLAYERS; j++) Race->points[j] = 0;

  putrace(Race);

  planet->info[Playernum - 1].numsectsowned = 1;
  planet->explored = 0;
  planet->info[Playernum - 1].explored = 1;
  /*planet->info[Playernum-1].autorep = 1;*/

  sect.owner = Playernum;
  sect.race = Playernum;
  sect.popn = planet->popn = Race->number_sexes;
  sect.fert = 100;
  sect.eff = 10;
  sect.troops = planet->troops = 0;
  planet->maxpopn =
      maxsupport(Race, sect, 100.0, 0) * planet->Maxx * planet->Maxy / 2;
  /* (approximate) */

  putsector(sect, *planet, secttypes[i].x, secttypes[i].y);
  putplanet(planet, Stars[star], pnum);

  /* make star explored and stuff */
  getstar(&Stars[star], star);
  setbit(Stars[star]->explored, Playernum);
  setbit(Stars[star]->inhabited, Playernum);
  Stars[star]->AP[Playernum - 1] = 5;
  putstar(Stars[star], star);
  close_data_files();

  sigprocmask(SIG_SETMASK, &mask, NULL);

  printf("\nYou are player %d.\n\n", Playernum);
  printf("Your race has been created on sector %d,%d on\n", secttypes[i].x,
         secttypes[i].y);
  printf("%s/%s.\n\n", Stars[star]->name, Stars[star]->pnames[pnum]);
  return 0;
}

static char desshow(planettype *p, int x, int y,
                    sector_map &smap) /* copied from map.c */
{
  auto &s = smap.get(x, y);

  switch (s.condition) {
    case WASTED:
      return CHAR_WASTED;
    case SEA:
      return CHAR_SEA;
    case LAND:
      return CHAR_LAND;
    case MOUNT:
      return CHAR_MOUNT;
    case GAS:
      return CHAR_GAS;
    case PLATED:
      return CHAR_PLATED;
    case DESERT:
      return CHAR_DESERT;
    case FOREST:
      return CHAR_FOREST;
    case ICE:
      return CHAR_ICE;
    default:
      return ('!');
  }
}

// TODO(jeffbailey): We shouldn't need to be providing this function.
int notify(int who, int gov, const char *msg) { /* this is a dummy routine */
  return 0;
}

// TODO(jeffbailey): We shouldn't need to be providing this function.
void warn(int, int, char *);
void warn(int who, int gov, char *msg) { /* this is a dummy routine */
}
