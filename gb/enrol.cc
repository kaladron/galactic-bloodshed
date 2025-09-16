// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* enrol.c -- initializes to owner one sector and planet. */

import std.compat;
import gblib;

#include <sqlite3.h>
#include <strings.h>
#include <unistd.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>

struct stype {
  char here;
  char x, y;
  int count;
};

#define RACIAL_TYPES 10

// TODO(jeffbailey): Copied from map.c, but they've diverged
static char desshow(const int x, const int y, SectorMap&);

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
  int pnum = 0;
  int star = 0;
  int found = 0;
  int check;
  int vacant;
  int count;
  int i;
  int j;
  player_t Playernum;
  PlanetType ppref;
  sigset_t mask;
  sigset_t block;
  int idx;
  int k;
#define STRSIZE 100
  char str[STRSIZE];
  char c;
  struct stype secttypes[SectorType::SEC_WASTED + 1] = {};
  Planet planet;
  unsigned char not_found[PlanetType::DESERT + 1];

  Sql db{};

  srandom(getpid());

  if ((Playernum = db.Numraces() + 1) >= MAXPLAYERS) {
    printf("There are already %d players; No more allowed.\n", MAXPLAYERS - 1);
    exit(-1);
  }

  printf("Enter racial type to be created (1-%d):", RACIAL_TYPES);
  if (scanf("%d", &idx) < 0) {
    perror("Cannot read input");
    exit(-1);
  }
  std::getchar();

  if (idx <= 0 || idx > RACIAL_TYPES) {
    printf("Bad racial index.\n");
    exit(1);
  }
  idx = idx - 1;

  db.getsdata(&Sdata);

  // TODO(jeffbailey): factor out routine for initialising this.
  stars.reserve(Sdata.numstars);
  for (i = 0; i < Sdata.numstars; i++) {
    auto s = db.getstar(i);
    stars.push_back(s);
  }
  printf("There is still space for player %d.\n", Playernum);

  bzero((char*)not_found, sizeof(not_found));
  do {
    printf(
        "\nLive on what type planet:\n     (e)arth, (g)asgiant, (m)ars, "
        "(i)ce, (w)ater, (d)esert, (f)orest? ");
    c = std::getchar();
    std::getchar();

    switch (c) {
      case 'w':
        ppref = PlanetType::WATER;
        break;
      case 'e':
        ppref = PlanetType::EARTH;
        break;
      case 'm':
        ppref = PlanetType::MARS;
        break;
      case 'g':
        ppref = PlanetType::GASGIANT;
        break;
      case 'i':
        ppref = PlanetType::ICEBALL;
        break;
      case 'd':
        ppref = PlanetType::DESERT;
        break;
      case 'f':
        ppref = PlanetType::FOREST;
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
      if (stars[star].inhabited() != 0 || stars[star].numplanets() < 2)
        check = 0;

      /* look for uninhabited planets */
      if (check) {
        pnum = 0;
        while (!found && pnum < stars[star].numplanets()) {
          planet = getplanet(star, pnum);

          if (planet.type == ppref && stars[star].numplanets() != 1) {
            vacant = 1;
            for (i = 1; i <= Playernum; i++)
              if (planet.info[i - 1].numsectsowned) vacant = 0;
            if (vacant && planet.conditions[RTEMP] >= -50 &&
                planet.conditions[RTEMP] <= 50) {
              found = 1;
            }
          }
          if (!found) {
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
      for (found = 1, i = PlanetType::EARTH; i <= PlanetType::DESERT; i++)
        found &= not_found[i];
      if (found) {
        printf("Looks like there aren't any free planets left.  bye..\n");
        exit(-1);
      } else
        printf("  Try a different one...\n");
      found = 0;
    }

  } while (!found);

  Race race;
  bzero(&race, sizeof(Race));

  printf("\n\tDeity/Guest/Normal (d/g/n) ?");
  c = std::getchar();
  std::getchar();

  race.God = (c == 'd');
  race.Guest = (c == 'g');
  strcpy(race.name, "Unknown");

  // TODO(jeffbailey): What initializes the rest of the governors?
  race.governor[0].money = 0;
  race.governor[0].homelevel = race.governor[0].deflevel =
      ScopeLevel::LEVEL_PLAN;
  race.governor[0].homesystem = race.governor[0].defsystem = star;
  race.governor[0].homeplanetnum = race.governor[0].defplanetnum = pnum;
  /* display options */
  race.governor[0].toggle.highlight = Playernum;
  race.governor[0].toggle.inverse = 1;
  race.governor[0].toggle.color = 0;
  race.governor[0].active = 1;
  printf("Enter the password for this race:");
  if (scanf("%s", race.password) < 0) {
    perror("Cannot read input");
    exit(-1);
  }
  std::getchar();
  printf("Enter the password for this leader:");
  if (scanf("%s", race.governor[0].password) < 0) {
    perror("Cannot read input");
    exit(-1);
  }
  std::getchar();

  /* make conditions preferred by your people set to (more or less)
     those of the planet : higher the concentration of gas, the higher
     percentage difference between planet and race (commented out) */
  for (j = 0; j <= OTHER; j++) race.conditions[j] = planet.conditions[j];
  /*+ int_rand( round_rand(-planet->conditions[j]*2.0),
   * round_rand(planet->conditions[j]*2.0) )*/

  for (i = 0; i < MAXPLAYERS; i++) {
    /* messages from autoreport, player #1 are decodable */
    if ((i == (Playernum - 1) || Playernum == 1) || race.God)
      race.translate[i] = 100; /* you can talk to own race */
    else
      race.translate[i] = 1;
  }

  /* assign racial characteristics */
  for (i = 0; i < NUM_DISCOVERIES; i++) race.discoveries[i] = 0;
  race.tech = 0.0;
  race.morale = 0;
  race.turn = 0;
  race.allied = 0;
  race.atwar = 0;
  do {
    race.mass = RMass(idx);
    race.birthrate = Birthrate(idx);
    race.fighters = Fighters(idx);
    if (Thing[idx]) {
      race.IQ = 0;
      race.Metamorph = race.absorb = race.collective_iq = race.pods = true;
    } else {
      race.IQ = Intelligence(idx);
      race.Metamorph = race.absorb = race.collective_iq = race.pods = false;
    }
    race.adventurism = Adventurism(idx);
    race.number_sexes = Sexes(idx);
    race.metabolism = Metabolism(idx);

    printf("%s\n", race.Metamorph ? "METAMORPHIC" : "");
    printf("       Birthrate: %.3f\n", race.birthrate);
    printf("Fighting ability: %d\n", race.fighters);
    printf("              IQ: %d\n", race.IQ);
    printf("      Metabolism: %.2f\n", race.metabolism);
    printf("     Adventurism: %.2f\n", race.adventurism);
    printf("            Mass: %.2f\n", race.mass);
    printf(" Number of sexes: %d (min req'd for colonization)\n",
           race.number_sexes);

    printf("\n\nLook OK(y/n)\?");
    if (fgets(str, STRSIZE, stdin) == nullptr) exit(1);
  } while (str[0] != 'y');

  auto smap = getsmap(planet);

  printf(
      "\nChoose a primary sector preference. This race will prefer to "
      "live\non this type of sector.\n");

  for (auto shuffled = smap.shuffle(); auto& sector_wrap : shuffled) {
    Sector& sector = sector_wrap;
    secttypes[sector.condition].count++;
    if (!secttypes[sector.condition].here) {
      secttypes[sector.condition].here = 1;
      secttypes[sector.condition].x = sector.x;
      secttypes[sector.condition].y = sector.y;
    }
  }
  planet.explored = 1;
  for (i = SectorType::SEC_SEA; i <= SectorType::SEC_WASTED; i++)
    if (secttypes[i].here) {
      printf("(%2d): %c (%d, %d) (%s, %d sectors)\n", i,
             desshow(secttypes[i].x, secttypes[i].y, smap), secttypes[i].x,
             secttypes[i].y, Desnames[i], secttypes[i].count);
    }
  planet.explored = 0;

  found = 0;
  do {
    printf("\nchoice (enter the number): ");
    if (scanf("%d", &i) < 0) {
      perror("Cannot read input");
      exit(-1);
    }
    std::getchar();
    if (i < SectorType::SEC_SEA || i > SectorType::SEC_WASTED ||
        !secttypes[i].here) {
      printf("There are none of that type here..\n");
    } else
      found = 1;
  } while (!found);

  auto& sect = smap.get(secttypes[i].x, secttypes[i].y);
  race.likesbest = i;
  race.likes[i] = 1.0;
  race.likes[SectorType::SEC_PLATED] = 1.0;
  race.likes[SectorType::SEC_WASTED] = 0.0;
  printf("\nEnter compatibilities of other sectors -\n");
  for (j = SectorType::SEC_SEA; j < SectorType::SEC_PLATED; j++)
    if (i != j) {
      printf("%6s (%3d sectors) :", Desnames[j], secttypes[j].count);
      if (scanf("%d", &k) < 0) {
        perror("Cannot read input");
        exit(-1);
      }
      race.likes[j] = (double)k / 100.0;
    }
  printf("Numraces = %d\n", db.Numraces());
  Playernum = race.Playernum = db.Numraces() + 1;

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
    Ship s;
    int shipno;

    bzero(&s, sizeof(s));
    shipno = Numships() + 1;
    printf("Creating government ship %d...\n", shipno);
    race.Gov_ship = shipno;
    planet.ships = shipno;
    s.nextship = 0;

    s.type = ShipType::OTYPE_GOV;
    s.xpos = stars[star].xpos() + planet.xpos;
    s.ypos = stars[star].ypos() + planet.ypos;
    s.land_x = (char)secttypes[i].x;
    s.land_y = (char)secttypes[i].y;

    s.speed = 0;
    s.owner = Playernum;
    s.race = Playernum;
    s.governor = 0;

    s.tech = 100.0;

    s.build_type = ShipType::OTYPE_GOV;
    s.armor = Shipdata[ShipType::OTYPE_GOV][ABIL_ARMOR];
    s.guns = PRIMARY;
    s.primary = Shipdata[ShipType::OTYPE_GOV][ABIL_GUNS];
    s.primtype = Shipdata[ShipType::OTYPE_GOV][ABIL_PRIMARY];
    s.secondary = Shipdata[ShipType::OTYPE_GOV][ABIL_GUNS];
    s.sectype = Shipdata[ShipType::OTYPE_GOV][ABIL_SECONDARY];
    s.max_crew = Shipdata[ShipType::OTYPE_GOV][ABIL_MAXCREW];
    s.max_destruct = Shipdata[ShipType::OTYPE_GOV][ABIL_DESTCAP];
    s.max_resource = Shipdata[ShipType::OTYPE_GOV][ABIL_CARGO];
    s.max_fuel = Shipdata[ShipType::OTYPE_GOV][ABIL_FUELCAP];
    s.max_speed = Shipdata[ShipType::OTYPE_GOV][ABIL_SPEED];
    s.build_cost = Shipdata[ShipType::OTYPE_GOV][ABIL_COST];
    s.size = 100;
    s.base_mass = 100.0;
    sprintf(s.shipclass, "Standard");

    s.fuel = 0.0;
    s.popn = Shipdata[s.type][ABIL_MAXCREW];
    s.troops = 0;
    s.mass = s.base_mass + Shipdata[s.type][ABIL_MAXCREW] * race.mass;
    s.destruct = s.resource = 0;

    s.alive = 1;
    s.active = 1;
    s.protect.self = 1;

    s.docked = 1;
    /* docked on the planet */
    s.whatorbits = ScopeLevel::LEVEL_PLAN;
    s.whatdest = ScopeLevel::LEVEL_PLAN;
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
    std::cout << std::format("Created on sector {},{} on /{}/{}\n", s.land_x,
                             s.land_y, stars[s.storbits].get_name(),
                             stars[s.storbits].get_planet_name(s.pnumorbits));
    db.putship(&s);
  }

  for (j = 0; j < MAXPLAYERS; j++) race.points[j] = 0;

  db.putrace(race);

  planet.info[Playernum - 1].numsectsowned = 1;
  planet.explored = 0;
  planet.info[Playernum - 1].explored = 1;
  /*planet->info[Playernum-1].autorep = 1;*/

  sect.owner = Playernum;
  sect.race = Playernum;
  sect.popn = planet.popn = race.number_sexes;
  sect.fert = 100;
  sect.eff = 10;
  sect.troops = planet.troops = 0;
  planet.maxpopn =
      maxsupport(race, sect, 100.0, 0) * planet.Maxx * planet.Maxy / 2;
  /* (approximate) */

  putsector(sect, planet, secttypes[i].x, secttypes[i].y);
  putplanet(planet, stars[star], pnum);

  /* make star explored and stuff */
  stars[star] = db.getstar(star);
  setbit(stars[star].explored(), Playernum);
  setbit(stars[star].inhabited(), Playernum);
  stars[star].AP(Playernum - 1) = 5;
  db.putstar(stars[star], star);

  sigprocmask(SIG_SETMASK, &mask, nullptr);

  printf("\nYou are player %d.\n\n", Playernum);
  printf("Your race has been created on sector %d,%d on\n", secttypes[i].x,
         secttypes[i].y);
  std::cout << std::format("{}/{}.\n\n", stars[star].get_name(),
                           stars[star].get_planet_name(pnum));
  return 0;
}

static char desshow(const int x, const int y,
                    SectorMap& smap) /* copied from map.c */
{
  const auto& s = smap.get(x, y);

  switch (s.condition) {
    case SectorType::SEC_WASTED:
      return CHAR_WASTED;
    case SectorType::SEC_SEA:
      return CHAR_SEA;
    case SectorType::SEC_LAND:
      return CHAR_LAND;
    case SectorType::SEC_MOUNT:
      return CHAR_MOUNT;
    case SectorType::SEC_GAS:
      return CHAR_GAS;
    case SectorType::SEC_PLATED:
      return CHAR_PLATED;
    case SectorType::SEC_DESERT:
      return CHAR_DESERT;
    case SectorType::SEC_FOREST:
      return CHAR_FOREST;
    case SectorType::SEC_ICE:
      return CHAR_ICE;
    default:
      return ('!');
  }
}
