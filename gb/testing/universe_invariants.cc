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
  // 1. Star APs >= 0 for all currently registered races
  for (const Star& star : StarList::readonly(em)) {
    for (const Race& race : RaceList::readonly(em)) {
      expect_ge(star.AP(race.Playernum), 0,
                std::format("Star '{}' has negative AP for race '{}'",
                            star.get_name(), race.name),
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

  // 3. Ships have valid numbers, valid owner (if alive), and owner <=
  // MAXPLAYERS
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
    }
  }

  // 4. Commodities have valid owner <= MAXPLAYERS
  for (const Commod& commod : CommodList::readonly(em)) {
    if (commod.owner.value > 0) {
      expect_le(commod.owner.value, MAXPLAYERS,
                std::format("Commodity #{} has owner {} > MAXPLAYERS",
                            commod.id, commod.owner.value),
                loc);
    }
  }
}

}  // namespace test
