// SPDX-License-Identifier: Apache-2.0

/// \file universe_invariants.cc
/// \brief Implementation of cross-entity integrity invariant verification.

module;

#include <cassert>

module test;

import commands;
import dallib;
import gb.entities;
import gb.services;
import gb.repositories;
import std;

namespace test {

void verify_universe_invariants(EntityManager& em, std::source_location loc) {
  // 1. Star APs >= 0 and governor >= 1 for all currently registered races
  for (const Star& star : StarList::readonly(em)) {
    for (const Race& race : RaceList::readonly(em)) {
      expect_ge(star.AP(race.Playernum), 0,
                std::format("Star '{}' has negative AP for race '{}'",
                            star.get_name(), race.name),
                loc);
      expect_ge(star.governor(race.Playernum).value, 1,
                std::format("Star '{}' has invalid governor {} for race '{}'",
                            star.get_name(),
                            star.governor(race.Playernum).value, race.name),
                loc);
    }
  }

  // 1b. Races have valid leader (governor 1) and all active governors >= 1
  for (const Race& race : RaceList::readonly(em)) {
    expect_true(
        race.has_governor(Race::leader_id),
        std::format("Race '{}' is missing leader (governor 1)", race.name),
        loc);
    for (auto [gov_id, _] : race.active_governors()) {
      expect_ge(gov_id.value, 1,
                std::format("Race '{}' has invalid governor {}", race.name,
                            gov_id.value),
                loc);
    }
  }

  // 2. Planet population == sum(Sector populations) using range-based SectorMap
  for (const Star& star : StarList::readonly(em)) {
    for (const Planet& planet :
         PlanetList::readonly(em, star.star_id(), star)) {
      try {
        if (const auto* smap =
                em.peek_sectormap(planet.star_id(), planet.planet_order())) {
          population_t total_sect_pop = 0;
          for (const Sector& sect : *smap) {
            total_sect_pop += sect.get_popn();
          }
          expect_eq(
              planet.popn(), total_sect_pop,
              std::format("Planet ({}, {}) population mismatch with sector sum",
                          planet.star_id(), planet.planet_order()),
              loc);
        }
      } catch (const EntityNotFoundError&) {
        // SectorMap may not exist for uninitialized test planets
      }
    }
  }

  // 3. Ships have valid numbers, valid owner (if alive), owner <= MAXPLAYERS,
  // and governor >= 1
  for (const Ship& ship :
       ShipList::readonly(em, ShipList::IterationType::All)) {
    if (ship.alive()) {
      expect_ge(
          ship.owner().value, 1,
          std::format("Alive ship #{} has invalid owner 0", ship.number()),
          loc);
      expect_le(ship.owner().value, MAXPLAYERS,
                std::format("Alive ship #{} has owner {} > MAXPLAYERS",
                            ship.number(), ship.owner().value),
                loc);
      expect_ge(ship.governor().value, 1,
                std::format("Alive ship #{} has invalid governor {}",
                            ship.number(), ship.governor().value),
                loc);
    }
  }

  // 4. Commodities have valid owner <= MAXPLAYERS and governor >= 1
  for (const Commod& commod : CommodList::readonly(em)) {
    if (commod.owner.value > 0) {
      expect_le(commod.owner.value, MAXPLAYERS,
                std::format("Commodity #{} has owner {} > MAXPLAYERS",
                            commod.id, commod.owner.value),
                loc);
      expect_ge(commod.governor.value, 1,
                std::format("Commodity #{} has invalid governor {}", commod.id,
                            commod.governor.value),
                loc);
      if (commod.bidder.has_value()) {
        expect_ge(commod.bidder_gov.value, 1,
                  std::format("Commodity #{} has invalid bidder_gov {}",
                              commod.id, commod.bidder_gov.value),
                  loc);
      }
    }
  }
}

}  // namespace test
