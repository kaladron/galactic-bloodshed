// SPDX-License-Identifier: Apache-2.0

module;

import std;

module gblib;

static const std::array<int, 8> x_adj = {-1, 0, 1, -1, 1, -1, 0, 1};
static const std::array<int, 8> y_adj = {1, 1, 1, 0, 0, -1, -1, -1};

namespace {
void Migrate2(const Planet &planet, int xd, int yd, Sector &ps,
              population_t *people, SectorMap &smap) {
  /* attempt to migrate beyond screen, or too many people */
  if (yd > planet.Maxy() - 1 || yd < 0) return;

  if (xd < 0)
    xd = planet.Maxx() - 1;
  else if (xd > planet.Maxx() - 1)
    xd = 0;

  auto &pd = smap.get(xd, yd);

  if (!pd.owner) {
    int move = (int)((double)(*people) * Compat[ps.owner - 1] *
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

void plate(Sector &s) {
  s.eff = 100;
  if (s.condition != SectorType::SEC_GAS) s.condition = SectorType::SEC_PLATED;
}

// Process resource production from a sector
void processResourceProduction(const Race &race, Sector &s) {
  if (!s.resource || !success(s.eff)) return;

  resource_t prod = static_cast<resource_t>(round_rand(race.metabolism)) *
                    static_cast<resource_t>(int_rand(1, s.eff));
  prod = std::min(prod, s.resource);
  s.resource -= prod;

  auto pfuel = prod * (1 + (s.condition == SectorType::SEC_GAS));
  int owner_idx = s.owner - 1;

  if (success(s.mobilization)) {
    prod_destruct[owner_idx] += prod;
  } else {
    prod_res[owner_idx] += prod;
  }

  prod_fuel[owner_idx] += pfuel;
}

// Process crystal mining in a sector
void processCrystalMining(const Race &race, Sector &s) {
  if (s.crystals && Crystal(race) && success(s.eff)) {
    prod_crystals[s.owner - 1]++;
    s.crystals--;
  }
}

// Update sector mobilization based on planetary settings
void updateMobilization(Sector &s, const plinfo &pinf) {
  int owner_idx = s.owner - 1;

  if (s.mobilization < pinf.mob_set) {
    if (pinf.resource + prod_res[owner_idx] > 0) {
      s.mobilization++;
      prod_res[owner_idx] -= round_rand(MOB_COST);
      prod_mob++;
    }
  } else if (s.mobilization > pinf.mob_set) {
    s.mobilization--;
    prod_mob--;
  }

  avg_mob[owner_idx] += s.mobilization;
}

// Update sector efficiency and plating
void updateEfficiency(Sector &s, const Race &race, const Planet &planet) {
  if (s.eff < 100) {
    int chance = round_rand((100.0 - (double)planet.info(s.owner - 1).tax) *
                            race.likes[s.condition]);
    if (success(chance)) {
      s.eff += round_rand(race.metabolism);
      if (s.eff >= 100) plate(s);
    }
  } else {
    plate(s);
  }
}

// Update sector fertility and condition
void updateFertilityAndCondition(Sector &s, const Race &race) {
  if ((s.condition != SectorType::SEC_WASTED) && race.fertilize &&
      (s.fert < 100)) {
    s.fert += (int_rand(0, 100) < race.fertilize);
  }

  s.fert = std::min<int>(s.fert, 100);

  if (s.condition == SectorType::SEC_WASTED && success(NATURAL_REPAIR)) {
    s.condition = s.type;
  }
}

// Calculate population change based on sector conditions
population_t calculatePopulationChange(const Race &race, const Sector &s,
                                       population_t maxsup) {
  population_t diff = s.popn - maxsup;

  if (diff < 0) {
    if (s.popn >= race.number_sexes) {
      return round_rand(-static_cast<double>(diff) * race.birthrate);
    }
    return 0;
  }
  return -int_rand(0, std::min(2 * diff, s.popn));
}

// Handle population changes and owner updates
void updatePopulationAndOwner(Sector &s, const Race &race, const Star &star,
                              const Planet &planet) {
  auto maxsup =
      maxsupport(race, s, Compat[s.owner - 1], planet.conditions(TOXIC));
  s.popn += calculatePopulationChange(race, s, maxsup);

  // Handle troops maintenance costs - we have to modify global state here
  if (s.troops &&
      races[s.owner - 1].governor[star.governor(s.owner - 1)].maintain) {
    races[s.owner - 1].governor[star.governor(s.owner - 1)].maintain +=
        UPDATE_TROOP_COST * s.troops;
  }

  // Update ownership if no population remains
  if (!s.popn && !s.troops) {
    s.owner = 0;
  }
}
}  // anonymous namespace

//  produce() -- produce, stuff like that, on a sector.
void produce(const Star &star, const Planet &planet, Sector &s) {
  if (!s.owner) return;
  auto &race = races[s.owner - 1];

  // Process production and resources
  processResourceProduction(race, s);
  processCrystalMining(race, s);

  // Handle mobilization
  const auto &pinf = planet.info(s.owner - 1);
  updateMobilization(s, pinf);

  // Update efficiency, fertility and sector condition
  updateEfficiency(s, race, planet);
  updateFertilityAndCondition(s, race);

  // Handle population changes and ownership
  updatePopulationAndOwner(s, race, star, planet);
}

// spread()  -- spread population around.
void spread(const Planet &pl, Sector &s, SectorMap &smap) {
  if (!s.owner) return;
  if (pl.slaved_to() && pl.slaved_to() != s.owner)
    return; /* no one wants to go anywhere */

  auto &race = races[s.owner - 1];

  /* the higher the fertility, the less people like to leave */
  population_t people =
      round_rand(race.adventurism * static_cast<double>(s.popn) *
                 (100. - s.fert) / 100.) -
      race.number_sexes; /* how many people want to move -
                                          one family stays behind */

  int check = round_rand(6.0 * race.adventurism); /* more rounds for
                                                               high advent */
  while (people > 0 && check) {
    int j = int_rand(0, 7);
    int x2 = x_adj[j];
    int y2 = y_adj[j];
    Migrate2(pl, s.x + x2, s.y + y2, s, &people, smap);
    check--;
  }
}

/* mark sectors on the planet as having been "explored." for sea exploration
        on earthtype planets.  */

//  explore() -- mark sector and surrounding sectors as having been explored.
void explore(const Planet &planet, Sector &s, int x, int y, int p) {
  // explore sectors surrounding sectors currently explored.
  if (Sectinfo[x][y].explored) {
    Sectinfo[mod(x - 1, planet.Maxx())][y].explored = p;
    Sectinfo[mod(x + 1, planet.Maxx())][y].explored = p;
    if (y == 0) {
      Sectinfo[x][1].explored = p;
    } else if (y == planet.Maxy() - 1) {
      Sectinfo[x][y - 1].explored = p;
    } else {
      Sectinfo[x][y - 1].explored = Sectinfo[x][y + 1].explored = p;
    }
  } else if (s.owner == p) {
    Sectinfo[x][y].explored = p;
  }
}
