// SPDX-License-Identifier: Apache-2.0

/// \file dosector.cc
/// \brief Planetary surface sector turn simulation processing.

module;

import std;
#undef stdout

module gblib;

/// \brief Computes how many colonists migrate to an unowned adjacent target
/// sector.
/// \param race Species traits and environmental preferences.
/// \param compatibility Planet habitability compatibility factor for the
/// species.
/// \param target Destination sector to migrate into.
/// \param available_migrants Current pool of colonists seeking migration.
/// \return Number of colonists moving to the target sector.
population_t calculate_migrating_colonists(const Race& race,
                                           double compatibility,
                                           const Sector& target,
                                           population_t available_migrants) {
  if (available_migrants <= 0 || target.is_owned()) {
    return 0;
  }
  const double likes_factor = race.likes[target.get_condition()];
  const double move_calc = static_cast<double>(available_migrants) *
                           compatibility * likes_factor / 100.0;
  return std::clamp(std::lround(move_calc), population_t{0},
                    available_migrants);
}

/// \brief Attempts to migrate colonists from a source sector to an adjacent
/// target coordinate.
/// \param entity_manager Reference to the game entity manager.
/// \param planet Planet hosting the sectors.
/// \param source Populated source sector.
/// \param target_coords Grid coordinates of the target sector.
/// \param available_migrants Available pool of potential migrating colonists.
/// \param smap Planetary sector grid.
/// \param stats Turn statistics to update upon successful colonization.
/// \return Number of colonists successfully transferred.
population_t attempt_colonist_migration(EntityManager& entity_manager,
                                        const Planet& planet, Sector& source,
                                        Coordinates target_coords,
                                        population_t available_migrants,
                                        SectorMap& smap, TurnStats& stats) {
  if (available_migrants <= 0 || !planet.is_valid(target_coords)) {
    return 0;
  }
  auto& target_sector = smap.get(target_coords);
  if (target_sector.is_owned()) {
    return 0;
  }
  return entity_manager.with_race(
      source.get_owner(), [&](const Race& race) -> population_t {
        const population_t move = calculate_migrating_colonists(
            race, stats.Compat[source.get_owner()], target_sector,
            available_migrants);
        if (move <= 0) {
          return 0;
        }
        source.transfer_popn_to(target_sector, move);
        stats.tot_captured++;
        stats.Claims = true;
        return move;
      });
}

namespace {

// Process resource production from a sector
void processResourceProduction(const Race& race, Sector& s, TurnStats& stats) {
  if (!s.get_resource() || !success(s.get_eff())) return;

  resource_t prod = static_cast<resource_t>(round_rand(race.metabolism)) *
                    static_cast<resource_t>(int_rand(1, s.get_eff()));
  prod = std::min(prod, s.get_resource());
  s.set_resource(s.get_resource() - prod);

  auto pfuel = prod * (1 + (s.get_condition() == SectorType::SEC_GAS));
  player_t owner = s.get_owner();

  if (success(s.get_mobilization())) {
    stats.prod_destruct[owner] += prod;
  } else {
    stats.prod_res[owner] += prod;
  }

  stats.prod_fuel[owner] += pfuel;
}

// Process crystal mining in a sector
void processCrystalMining(const Race& race, Sector& s, TurnStats& stats) {
  if (s.get_crystals() && race.discoveries.crystal && success(s.get_eff())) {
    stats.prod_crystals[s.get_owner()]++;
    s.set_crystals(s.get_crystals() - 1);
  }
}

// Update sector mobilization based on planetary settings
void updateMobilization(Sector& s, const plinfo& pinf, TurnStats& stats) {
  player_t owner = s.get_owner();

  if (s.get_mobilization() < pinf.mob_set) {
    if (pinf.resource + stats.prod_res[owner] > 0) {
      s.adjust_mobilization(1);
      stats.prod_res[owner] -= round_rand(MOB_COST);
      stats.prod_mob++;
    }
  } else if (s.get_mobilization() > pinf.mob_set) {
    s.adjust_mobilization(-1);
    stats.prod_mob--;
  }

  stats.avg_mob[owner] += s.get_mobilization();
}

// Update sector efficiency and plating
void updateEfficiency(Sector& s, const Race& race, const Planet& planet) {
  if (s.get_eff() < 100) {
    int chance = round_rand((100.0 - (double)planet.info(s.get_owner()).tax) *
                            race.likes[s.get_condition()]);
    if (success(chance)) {
      s.improve_efficiency(round_rand(race.metabolism));
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
                              const Planet& planet, TurnStats& stats) {
  auto maxsup = maxsupport(race, s, stats.Compat[s.get_owner()],
                           planet.conditions(TOXIC));
  s.add_popn(calculatePopulationChange(race, s, maxsup));

  // Handle troops maintenance costs - mutate race for governor update
  if (s.get_troops()) {
    entity_manager.mutate_race(s.get_owner(), [&](Race& r) {
      r.governor[star.governor(s.get_owner()).value].maintain +=
          UPDATE_TROOP_COST * s.get_troops();
    });
  }

  // Update ownership if no population remains
  s.clear_owner_if_empty();
}
}  // anonymous namespace

/// \brief Runs industrial production, resource extraction, and sector growth on
/// a sector.
/// \param entity_manager Reference to the game entity manager.
/// \param star Star hosting the planetary system.
/// \param planet Planet hosting the sector.
/// \param s Planetary sector to simulate.
/// \param stats Empire-wide simulation turn statistics.
void produce(EntityManager& entity_manager, const Star& star,
             const Planet& planet, Sector& s, TurnStats& stats) {
  if (!s.is_owned()) return;

  entity_manager.with_race(s.get_owner(), [&](const Race& race) {
    // Process production and resources
    processResourceProduction(race, s, stats);
    processCrystalMining(race, s, stats);

    // Handle mobilization
    const auto& pinf = planet.info(s.get_owner());
    updateMobilization(s, pinf, stats);

    // Update efficiency, fertility and sector condition
    updateEfficiency(s, race, planet);
    updateFertilityAndCondition(s, race);

    // Handle population changes and ownership
    updatePopulationAndOwner(entity_manager, s, race, star, planet, stats);
  });
}

/// \brief Spreads sector population across adjacent unowned planetary sectors
/// during turn updates.
/// \param entity_manager Reference to the game entity manager.
/// \param pl Planet hosting the sectors.
/// \param s Source sector with population.
/// \param smap Planetary sector grid.
/// \param stats Simulation turn statistics.
void spread(EntityManager& entity_manager, const Planet& pl, Sector& s,
            SectorMap& smap, TurnStats& stats) {
  if (!s.is_owned()) return;
  if (pl.slaved_to() != 0 && pl.slaved_to() != s.get_owner()) {
    return; /* no one wants to go anywhere */
  }

  entity_manager.with_race(s.get_owner(), [&](const Race& race) {
    /* the higher the fertility, the less people like to leave */
    const double base_migrants = race.adventurism *
                                 static_cast<double>(s.get_popn()) *
                                 (100.0 - s.get_fert()) / 100.0;
    const auto raw_migrants = round_rand(base_migrants);
    if (raw_migrants <= race.number_sexes) {
      return;
    }
    population_t people =
        raw_migrants - race.number_sexes; /* one family stays behind */

    const auto neighbors = pl.adjacent_coordinates(s.coords());
    if (neighbors.empty()) {
      return;
    }

    int check =
        round_rand(6.0 * race.adventurism); /* more rounds for high advent */
    while (people > 0 && check > 0) {
      const auto target_coords = neighbors[int_rand(0, neighbors.size() - 1)];
      const population_t moved = attempt_colonist_migration(
          entity_manager, pl, s, target_coords, people, smap, stats);
      people -= moved;
      --check;
    }
  });
}
