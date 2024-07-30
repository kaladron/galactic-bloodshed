// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std.compat;

#include "gb/GB_racegen.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "gb/GB_server.h"
#include "gb/files_shl.h"
#include "gb/globals.h"
#include "gb/max.h"
#include "gb/racegen.h"
#include "gb/races.h"
#include "gb/shipdata.h"
#include "gb/ships.h"
#include "gb/sql/sql.h"
#include "gb/tweakables.h"
#include "gb/vars.h"

static const PlanetType planet_translate[N_HOME_PLANET_TYPES] = {
    PlanetType::EARTH,   PlanetType::FOREST, PlanetType::DESERT,
    PlanetType::WATER,   PlanetType::MARS,   PlanetType::ICEBALL,
    PlanetType::GASGIANT};

/* this is a dummy routine */
bool notify(const player_t, const governor_t, const std::string &) {
  return false;
}

/* this is a dummy routine */
void warn(const player_t, const governor_t, const std::string &) {}

void init_enroll() { srandom(getpid()); }

/*
 * Returns 0 if successfully enrolled, or 1 if failure. */
int enroll_valid_race() {
  int star;
  int pnum;
  int i;
  player_t Playernum;
  PlanetType ppref;
  int last_star_left;
  int indirect[NUMSTARS];
  sigset_t mask;
  sigset_t block;
  Planet planet;
  /*
    if (race.status == STATUS_ENROLLED) {
      sprintf(race.rejection, "This race has already been enrolled!\n") ;
      return 1 ;
      }
  */
  Sql db{};
  Playernum = db.Numraces() + 1;
  if ((Playernum == 1) && (race_info.priv_type != P_GOD)) {
    sprintf(race_info.rejection,
            "The first race enrolled must have God privileges.\n");
    return 1;
  }
  if (Playernum >= MAXPLAYERS) {
    sprintf(race_info.rejection,
            "There are already %d players; No more allowed.\n", MAXPLAYERS - 1);
    race_info.status = STATUS_UNENROLLABLE;
    return 1;
  }

  getsdata(&Sdata);
  for (i = 0; i < Sdata.numstars; i++) {
    auto s = db.getstar(i);
    stars.push_back(s);
  }

  printf("Looking for %s..", planet_print_name[race_info.home_planet_type]);
  fflush(stdout);

  ppref = planet_translate[race_info.home_planet_type];
  for (i = 0; i < Sdata.numstars; i++) indirect[i] = i;
  last_star_left = Sdata.numstars - 1;
  while (last_star_left >= 0) {
    i = int_rand(0, last_star_left);
    star = indirect[i];

    printf(".");
    fflush(stdout);
    /*
     * Skip over inhabited stars and stars with few planets. */
    if ((stars[star].numplanets < 2) || stars[star].inhabited) {
    } else {
      /* look for uninhabited planets */
      for (pnum = 0; pnum < stars[star].numplanets; pnum++) {
        planet = getplanet(star, pnum);
        if ((planet.type == ppref) && (planet.conditions[RTEMP] >= -200) &&
            (planet.conditions[RTEMP] <= 100))
          goto found_planet;
      }
    }
    /*
     * Since we are here, this star didn't work out: */
    indirect[i] = indirect[last_star_left--];
  }

  /*
   * If we get here, then we did not find any good planet. */
  printf(" failed!\n");
  sprintf(race_info.rejection,
          "Didn't find any free %s; choose another home planet type.\n",
          planet_print_name[race_info.home_planet_type]);
  race_info.status = STATUS_UNENROLLABLE;
  return 1;

found_planet:
  printf(" found!\n");
  auto race = new Race;
  bzero(race, sizeof(Race));

  race->Playernum = Playernum;
  race->God = (race_info.priv_type == P_GOD);
  race->Guest = (race_info.priv_type == P_GUEST);
  strcpy(race->name, race_info.name);
  strcpy(race->password, race_info.password);

  strcpy(race->governor[0].password, "0");
  race->governor[0].homelevel = race->governor[0].deflevel =
      ScopeLevel::LEVEL_PLAN;
  race->governor[0].homesystem = race->governor[0].defsystem = star;
  race->governor[0].homeplanetnum = race->governor[0].defplanetnum = pnum;
  /* display options */
  race->governor[0].toggle.highlight = Playernum;
  race->governor[0].toggle.inverse = 1;
  race->governor[0].toggle.color = 0;
  race->governor[0].active = 1;

  for (i = 0; i <= OTHER; i++) race->conditions[i] = planet.conditions[i];
#if 0
  /* make conditions preferred by your people set to (more or less) 
     those of the planet : higher the concentration of gas, the higher
     percentage difference between planet and race */
  for (j=0; j<=OTHER; j++)
    race->conditions[j] = planet->conditions[j]
      + int_rand(round_rand(-planet->conditions[j]*2.0), 
		 round_rand(planet->conditions[j]*2.0) ) ;
#endif

  for (i = 1; i <= MAXPLAYERS; i++) {
    /* messages from autoreport, player #1 are decodable */
    if ((i == Playernum) || (Playernum == 1) || race->God)
      race->translate[i - 1] = 100; /* you can talk to own race */
    else
      race->translate[i - 1] = 1;
  }

#if 0
  /* All of the following zeros are not really needed, because the race
     was bzero'ed out above. */
  for (i=0; i<80; i++)
    race->discoveries[i] = 0;
  race->tech = 0.0;
  race->morale = 0;
  race->turn = 0;
  race->allied[0] = race->allied[1] = 0;
  race->atwar[0] = race->atwar[1] = 0;
  for (i=0; i<MAXPLAYERS; i++) 
    race->points[i]=0;
#endif

  /*
   * Assign racial characteristics. */
  race->absorb = race_info.attr[ABSORB];
  race->collective_iq = race_info.attr[COL_IQ];
  race->Metamorph = (race_info.race_type == R_METAMORPH);
  race->pods = race_info.attr[PODS];

  race->fighters = race_info.attr[FIGHT];
  if (race_info.attr[COL_IQ] == 1.0)
    race->IQ_limit = race_info.attr[A_IQ];
  else
    race->IQ = race_info.attr[A_IQ];
  race->number_sexes = race_info.attr[SEXES];

  race->fertilize = race_info.attr[FERT] * 100;

  race->adventurism = race_info.attr[ADVENT];
  race->birthrate = race_info.attr[BIRTH];
  race->mass = race_info.attr[MASS];
  race->metabolism = race_info.attr[METAB];

  /*
   * Assign sector compats and determine a primary sector type. */
  for (i = FIRST_SECTOR_TYPE; i <= LAST_SECTOR_TYPE; i++) {
    race->likes[i] = race_info.compat[i] / 100.0;
    if ((100 == race_info.compat[i]) &&
        (1.0 == planet_compat_cov[race_info.home_planet_type][i]))
      race->likesbest = i;
  }

  /*
   * Find sector to build capital on, and populate it: */
  auto smap = getsmap(planet);

  Sector *sect;
  bool found_sector = false;
  for (auto shuffled = smap.shuffle(); auto &sector_wrap : shuffled) {
    sect = &sector_wrap.get();
    if (sect->condition != race->likesbest) continue;
    found_sector = true;
    break;
  }
  // We default to putting the capital at 0,0 if we don't have a better choice
  if (!found_sector) sect = &smap.get(0, 0);
  sect->owner = Playernum;
  sect->race = Playernum;
  sect->popn = planet.popn = race->number_sexes;
  sect->fert = 100;
  sect->eff = 10;
  sect->troops = planet.troops = 0;

  race->governors = 0;

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
    race->Gov_ship = shipno;
    planet.ships = shipno;
    s.nextship = 0;

    s.type = ShipType::OTYPE_GOV;
    s.xpos = stars[star].xpos + planet.xpos;
    s.ypos = stars[star].ypos + planet.ypos;
    s.land_x = sect->x;
    s.land_y = sect->y;

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
    s.mass = s.base_mass + Shipdata[s.type][ABIL_MAXCREW] * race->mass;
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
    putship(&s);
  }

  planet.info[Playernum - 1].numsectsowned = 1;
  planet.explored = 0;
  planet.info[Playernum - 1].explored = 1;
  /*planet->info[Playernum-1].autorep = 1;*/

  planet.maxpopn =
      maxsupport(*race, *sect, 100.0, 0) * planet.Maxx * planet.Maxy / 2;
  /* (approximate) */

#ifdef STARTING_INVENTORY

  if (race->Metamorph)
    planet.info[Playernum - 1].resource += (START_RES - START_MESO_RES_DIFF);
  else
    planet.info[Playernum - 1].resource += START_RES;

  planet.info[Playernum - 1].fuel += START_FUEL;
  planet.info[Playernum - 1].destruct += START_DES;

#endif

  putrace(*race);
  putsector(*sect, planet);

  stars[star] = getstar(star);
  putplanet(planet, stars[star], pnum);

  /* make star explored and stuff */
  setbit(stars[star].explored, Playernum);
  setbit(stars[star].inhabited, Playernum);
  stars[star].AP[Playernum - 1] = 5;
  putstar(stars[star], star);

  sigprocmask(SIG_SETMASK, &mask, nullptr);

  printf("Player %d (%s) created on sector %d,%d on %s/%s.\n", Playernum,
         race_info.name, sect->x, sect->y, stars[star].name,
         stars[star].pnames[pnum]);
  race_info.status = STATUS_ENROLLED;
  return 0;
}
