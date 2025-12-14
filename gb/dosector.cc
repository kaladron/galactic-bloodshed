// SPDX-License-Identifier: Apache-2.0

module;

import std;

module gblib;

static const std::array<int, 8> x_adj = {-1, 0, 1, -1, 1, -1, 0, 1};
static const std::array<int, 8> y_adj = {1, 1, 1, 0, 0, -1, -1, -1};

namespace {
void Migrate2(EntityManager& entity_manager, const Planet& planet, int xd,
              int yd, Sector& ps, population_t* people, SectorMap& smap) {
  /* attempt to migrate beyond screen, or too many people */
  if (yd > planet.Maxy() - 1 || yd < 0) return;

  if (xd < 0)
    xd = planet.Maxx() - 1;
  else if (xd > planet.Maxx() - 1)
    xd = 0;

  auto& pd = smap.get(xd, yd);

  if (!pd.is_owned()) {
    const auto* race = entity_manager.peek_race(ps.get_owner());
    if (!race) return;
    double move_calc = (*people) * Compat[ps.get_owner() - 1] *
                       race->likes[pd.get_condition()] / 100.0;
    // Round and clamp to valid population_t range
    auto move = std::clamp(std::lround(move_calc), population_t{0},
                           std::numeric_limits<population_t>::max());
    if (!move) return;
    *people -= move;
    pd.set_popn(pd.get_popn() + move);
    ps.set_popn(ps.get_popn() - move);
    pd.set_owner(ps.get_owner());
    tot_captured++;
    Claims = 1;
  }
}

// Process resource production from a sector
void processResourceProduction(const Race& race, Sector& s) {
  if (!s.get_resource() || !success(s.get_eff())) return;

  resource_t prod = static_cast<resource_t>(round_rand(race.metabolism)) *
                    static_cast<resource_t>(int_rand(1, s.get_eff()));
  prod = std::min(prod, s.get_resource());
  s.set_resource(s.get_resource() - prod);

  auto pfuel = prod * (1 + (s.get_condition() == SectorType::SEC_GAS));
  int owner_idx = s.get_owner() - 1;

  if (success(s.get_mobilization())) {
    prod_destruct[owner_idx] += prod;
  } else {
    prod_res[owner_idx] += prod;
  }

  prod_fuel[owner_idx] += pfuel;
}

// Process crystal mining in a sector
void processCrystalMining(const Race& race, Sector& s) {
  if (s.get_crystals() && Crystal(race) && success(s.get_eff())) {
    prod_crystals[s.get_owner() - 1]++;
    s.set_crystals(s.get_crystals() - 1);
  }
}

// Update sector mobilization based on planetary settings
void updateMobilization(Sector& s, const plinfo& pinf) {
  int owner_idx = s.get_owner() - 1;

  if (s.get_mobilization() < pinf.mob_set) {
    if (pinf.resource + prod_res[owner_idx] > 0) {
      s.set_mobilization(s.get_mobilization() + 1);
      prod_res[owner_idx] -= round_rand(MOB_COST);
      prod_mob++;
    }
  } else if (s.get_mobilization() > pinf.mob_set) {
    s.set_mobilization(s.get_mobilization() - 1);
    prod_mob--;
  }

  avg_mob[owner_idx] += s.get_mobilization();
}

// Update sector efficiency and plating
void updateEfficiency(Sector& s, const Race& race, const Planet& planet) {
  if (s.get_eff() < 100) {
    int chance =
        round_rand((100.0 - (double)planet.info(s.get_owner() - 1).tax) *
                   race.likes[s.get_condition()]);
    if (success(chance)) {
      s.set_eff(s.get_eff() + round_rand(race.metabolism));
      if (s.get_eff() >= 100) s.plate();
    }
  } else {
    s.plate();
  }
}

// Update sector fertility and condition
void updateFertilityAndCondition(Sector& s, const Race& race) {
  if (!s.is_wasted() && race.fertilize && (s.get_fert() < 100)) {
    s.set_fert(s.get_fert() + (int_rand(0, 100) < race.fertilize));
  }

  s.set_fert(std::min<int>(s.get_fert(), 100));

  if (s.is_wasted() && success(NATURAL_REPAIR)) {
    s.set_condition(s.get_type());
  }
}

// Calculate population change based on sector conditions
population_t calculatePopulationChange(const Race& race, const Sector& s,
                                       population_t maxsup) {
  population_t diff = s.get_popn() - maxsup;

  if (diff < 0) {
    if (s.get_popn() >= race.number_sexes) {
      return round_rand(-static_cast<double>(diff) * race.birthrate);
    }
    return 0;
  }
  return -int_rand(0, std::min(2 * diff, s.get_popn()));
}

// Handle population changes and owner updates
void updatePopulationAndOwner(EntityManager& entity_manager, Sector& s,
                              const Race& race, const Star& star,
                              const Planet& planet) {
  auto maxsup =
      maxsupport(race, s, Compat[s.get_owner() - 1], planet.conditions(TOXIC));
  s.set_popn(s.get_popn() + calculatePopulationChange(race, s, maxsup));

  // Handle troops maintenance costs - get mutable race for governor update
  if (s.get_troops()) {
    auto race_handle = entity_manager.get_race(s.get_owner());
    if (race_handle.get() &&
        race_handle->governor[star.governor(s.get_owner() - 1)].maintain) {
      (*race_handle).governor[star.governor(s.get_owner() - 1)].maintain +=
          UPDATE_TROOP_COST * s.get_troops();
    }
  }

  // Update ownership if no population remains
  s.clear_owner_if_empty();
}
}  // anonymous namespace

//  produce() -- produce, stuff like that, on a sector.
void produce(EntityManager& entity_manager, const Star& star,
             const Planet& planet, Sector& s) {
  if (!s.is_owned()) return;
  const auto* race = entity_manager.peek_race(s.get_owner());
  if (!race) return;

  // Process production and resources
  processResourceProduction(*race, s);
  processCrystalMining(*race, s);

  // Handle mobilization
  const auto& pinf = planet.info(s.get_owner() - 1);
  updateMobilization(s, pinf);

  // Update efficiency, fertility and sector condition
  updateEfficiency(s, *race, planet);
  updateFertilityAndCondition(s, *race);

  // Handle population changes and ownership
  updatePopulationAndOwner(entity_manager, s, *race, star, planet);
}

// spread()  -- spread population around.
void spread(EntityManager& entity_manager, const Planet& pl, Sector& s,
            SectorMap& smap) {
  if (!s.is_owned()) return;
  if (pl.slaved_to() && pl.slaved_to() != s.get_owner())
    return; /* no one wants to go anywhere */

  const auto* race = entity_manager.peek_race(s.get_owner());
  if (!race) return;

  /* the higher the fertility, the less people like to leave */
  population_t people =
      round_rand(race->adventurism * static_cast<double>(s.get_popn()) *
                 (100. - s.get_fert()) / 100.) -
      race->number_sexes; /* how many people want to move -
                                          one family stays behind */

  int check = round_rand(6.0 * race->adventurism); /* more rounds for
                                                               high advent */
  while (people > 0 && check) {
    int j = int_rand(0, 7);
    int x2 = x_adj[j];
    int y2 = y_adj[j];
    Migrate2(entity_manager, pl, s.get_x() + x2, s.get_y() + y2, s, &people,
             smap);
    check--;
  }
}

/* mark sectors on the planet as having been "explored." for sea exploration
        on earthtype planets.  */

//  explore() -- mark sector and surrounding sectors as having been explored.
void explore(const Planet& planet, Sector& s, int x, int y, int p) {
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
  } else if (s.get_owner() == p) {
    Sectinfo[x][y].explored = p;
  }
}
