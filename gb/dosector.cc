// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/dosector.h"

#include "gb/doturn.h"
#include "gb/max.h"
#include "gb/races.h"
#include "gb/tweakables.h"
#include "gb/utils/rand.h"
#include "gb/vars.h"

static const int x_adj[] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const int y_adj[] = {1, 1, 1, 0, 0, -1, -1, -1};

static void Migrate2(const Planet &, int, int, Sector &, int *, SectorMap &);
static void plate(Sector &);

//  produce() -- produce, stuff like that, on a sector.
void produce(const Star &star, const Planet &planet, Sector &s) {
  int ss;
  int maxsup;
  int pfuel = 0;
  int pdes = 0;
  int pres = 0;
  resource_t prod;
  population_t diff;

  if (!s.owner) return;
  auto &race = races[s.owner - 1];

  if (s.resource && success(s.eff)) {
    prod = round_rand(race.metabolism) * int_rand(1, s.eff);
    prod = std::min(prod, s.resource);
    s.resource -= prod;
    pfuel = prod * (1 + (s.condition == SectorType::SEC_GAS));
    if (success(s.mobilization))
      pdes = prod;
    else
      pres = prod;
    prod_fuel[s.owner - 1] += pfuel;
    prod_res[s.owner - 1] += pres;
    prod_destruct[s.owner - 1] += pdes;
  }

  /* try to find crystals */
  /* chance of digging out a crystal depends on efficiency */
  if (s.crystals && Crystal(race) && success(s.eff)) {
    prod_crystals[s.owner - 1]++;
    s.crystals--;
  }
  const auto pinf = &planet.info[s.owner - 1];

  /* increase mobilization to planetary quota */
  if (s.mobilization < pinf->mob_set) {
    if (pinf->resource + prod_res[s.owner - 1] > 0) {
      s.mobilization++;
      prod_res[s.owner - 1] -= round_rand(MOB_COST);
      prod_mob++;
    }
  } else if (s.mobilization > pinf->mob_set) {
    s.mobilization--;
    prod_mob--;
  }

  avg_mob[s.owner - 1] += s.mobilization;

  /* do efficiency */
  if (s.eff < 100) {
    int chance;
    chance = round_rand((100.0 - (double)planet.info[s.owner - 1].tax) *
                        race.likes[s.condition]);
    if (success(chance)) {
      s.eff += round_rand(race.metabolism);
      if (s.eff >= 100) plate(s);
    }
  } else
    plate(s);

  if ((s.condition != SectorType::SEC_WASTED) && race.fertilize &&
      (s.fert < 100))
    s.fert += (int_rand(0, 100) < race.fertilize);
  if (s.fert > 100) s.fert = 100;

  if (s.condition == SectorType::SEC_WASTED && success(NATURAL_REPAIR))
    s.condition = s.type;

  maxsup = maxsupport(race, s, Compat[s.owner - 1], planet.conditions[TOXIC]);
  if ((diff = s.popn - maxsup) < 0) {
    if (s.popn >= race.number_sexes)
      ss = round_rand(-(double)diff * race.birthrate);
    else
      ss = 0;
  } else
    ss = -int_rand(0, std::min(2 * diff, s.popn));
  s.popn += ss;

  if (s.troops)
    race.governor[star.governor[s.owner - 1]].maintain +=
        UPDATE_TROOP_COST * s.troops;
  else if (!s.popn)
    s.owner = 0;
}

// spread()  -- spread population around.
void spread(const Planet &pl, Sector &s, SectorMap &smap) {
  int people;
  int x2;
  int y2;
  int j;
  int check;

  if (!s.owner) return;
  if (pl.slaved_to && pl.slaved_to != s.owner)
    return; /* no one wants to go anywhere */

  auto &race = races[s.owner - 1];

  /* the higher the fertility, the less people like to leave */
  people = round_rand((double)race.adventurism * (double)s.popn *
                      (100. - (double)s.fert) / 100.) -
           race.number_sexes; /* how many people want to move -
                                               one family stays behind */

  check = round_rand(6.0 * race.adventurism); /* more rounds for
                                                               high advent */
  while (people > 0 && check) {
    j = int_rand(0, 7);
    x2 = x_adj[j];
    y2 = y_adj[j];
    Migrate2(pl, s.x + x2, s.y + y2, s, &people, smap);
    check--;
  }
}

static void Migrate2(const Planet &planet, int xd, int yd, Sector &ps,
                     int *people, SectorMap &smap) {
  int move;

  /* attempt to migrate beyond screen, or too many people */
  if (yd > planet.Maxy - 1 || yd < 0) return;

  if (xd < 0)
    xd = planet.Maxx - 1;
  else if (xd > planet.Maxx - 1)
    xd = 0;

  auto &pd = smap.get(xd, yd);

  if (!pd.owner) {
    move = (int)((double)(*people) * Compat[ps.owner - 1] *
                 races[ps.owner - 1].likes[pd.condition] / 100.0);
    if (!move) return;
    *people -= move;
    pd.popn += move;
    ps.popn -= move;
    pd.owner = ps.owner;
    tot_captured++;
    Claims = 1;
  }
}

/* mark sectors on the planet as having been "explored." for sea exploration
        on earthtype planets.  */

//  explore() -- mark sector and surrounding sectors as having been explored.
void explore(const Planet &planet, Sector &s, int x, int y, int p) {
  int d;

  /* explore sectors surrounding sectors currently explored. */
  if (Sectinfo[x][y].explored) {
    Sectinfo[mod(x - 1, planet.Maxx, d)][y].explored = p;
    Sectinfo[mod(x + 1, planet.Maxx, d)][y].explored = p;
    if (y == 0) {
      Sectinfo[x][1].explored = p;
    } else if (y == planet.Maxy - 1) {
      Sectinfo[x][y - 1].explored = p;
    } else {
      Sectinfo[x][y - 1].explored = Sectinfo[x][y + 1].explored = p;
    }

  } else if (s.owner == p)
    Sectinfo[x][y].explored = p;
}

static void plate(Sector &s) {
  s.eff = 100;
  if (s.condition != SectorType::SEC_GAS) s.condition = SectorType::SEC_PLATED;
}
