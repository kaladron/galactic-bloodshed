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

  // 1b. Races have valid leader (governor 1), active governors >= 1,
  // block/power records, and valid alive Gov_ship if set
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
    expect_ne(em.peek_block(race.Playernum), nullptr,
              std::format("Race '{}' is missing block record", race.name), loc);
    expect_ne(em.peek_power(race.Playernum), nullptr,
              std::format("Race '{}' is missing power record", race.name), loc);
    if (race.Gov_ship.has_value()) {
      const auto* gov_ship = em.peek_ship(*race.Gov_ship);
      expect_ne(gov_ship, nullptr,
                std::format("Race '{}' references non-existent Gov_ship #{}",
                            race.name, *race.Gov_ship),
                loc);
      if (gov_ship != nullptr) {
        expect_true(gov_ship->alive(),
                    std::format("Race '{}' references dead Gov_ship #{}",
                                race.name, *race.Gov_ship),
                    loc);
      }
    }
  }

  // 2. Planet population and troops == sum(Sector populations and troops)
  for (const Star& star : StarList::readonly(em)) {
    for (const Planet& planet :
         PlanetList::readonly(em, star.star_id(), star)) {
      try {
        if (const auto* smap =
                em.peek_sectormap(planet.star_id(), planet.planet_order())) {
          population_t total_sect_pop = 0;
          population_t total_sect_troops = 0;
          for (const Sector& sect : *smap) {
            total_sect_pop += sect.get_popn();
            total_sect_troops += sect.get_troops();
          }
          expect_eq(
              planet.popn(), total_sect_pop,
              std::format("Planet ({}, {}) population mismatch with sector sum",
                          planet.star_id(), planet.planet_order()),
              loc);
          expect_eq(
              planet.troops(), total_sect_troops,
              std::format("Planet ({}, {}) troops mismatch with sector sum",
                          planet.star_id(), planet.planet_order()),
              loc);
        }
      } catch (const EntityNotFoundError&) {
        // SectorMap may not exist for uninitialized test planets
      }
    }
  }

  // 3. Ships are alive, have valid owner/governor, valid spatial coordinates,
  // and valid target ship references
  for (const Ship& ship :
       ShipList::readonly(em, ShipList::IterationType::All)) {
    expect_true(ship.alive(),
                std::format("Dead ship #{} present in ShipList", ship.number()),
                loc);
    expect_ge(ship.owner().value, 1,
              std::format("Ship #{} has invalid owner 0", ship.number()), loc);
    expect_ne(em.peek_race(ship.owner()), nullptr,
              std::format("Ship #{} has non-existent owner {}", ship.number(),
                          ship.owner().value),
              loc);
    expect_ge(ship.governor().value, 1,
              std::format("Ship #{} has invalid governor {}", ship.number(),
                          ship.governor().value),
              loc);

    switch (ship.whatorbits()) {
      case ScopeLevel::LEVEL_UNIV:
        expect_eq(ship.storbits(), starnum_t{0},
                  std::format("LEVEL_UNIV ship #{} has non-zero storbits {}",
                              ship.number(), ship.storbits()),
                  loc);
        expect_eq(ship.pnumorbits(), planetnum_t{0},
                  std::format("LEVEL_UNIV ship #{} has non-zero pnumorbits {}",
                              ship.number(), ship.pnumorbits()),
                  loc);
        break;
      case ScopeLevel::LEVEL_STAR:
        expect_ge(ship.storbits().value, 1,
                  std::format("LEVEL_STAR ship #{} has invalid storbits {}",
                              ship.number(), ship.storbits()),
                  loc);
        expect_eq(ship.pnumorbits(), planetnum_t{0},
                  std::format("LEVEL_STAR ship #{} has non-zero pnumorbits {}",
                              ship.number(), ship.pnumorbits()),
                  loc);
        break;
      case ScopeLevel::LEVEL_PLAN:
        expect_ge(ship.storbits().value, 1,
                  std::format("LEVEL_PLAN ship #{} has invalid storbits {}",
                              ship.number(), ship.storbits()),
                  loc);
        expect_ge(ship.pnumorbits().value, 1,
                  std::format("LEVEL_PLAN ship #{} has invalid pnumorbits {}",
                              ship.number(), ship.pnumorbits()),
                  loc);
        break;
      case ScopeLevel::LEVEL_SHIP:
        expect_true(ship.destshipno().has_value() &&
                        ship.destshipno()->value >= 1,
                    std::format("LEVEL_SHIP ship #{} missing valid destshipno",
                                ship.number()),
                    loc);
        break;
    }

    if (ship.destshipno().has_value()) {
      expect_ne(em.peek_ship(*ship.destshipno()), nullptr,
                std::format("Ship #{} has non-existent destshipno #{}",
                            ship.number(), *ship.destshipno()),
                loc);
    }
    if (ship.protect().ship.has_value()) {
      expect_ne(em.peek_ship(*ship.protect().ship), nullptr,
                std::format("Ship #{} protects non-existent ship #{}",
                            ship.number(), *ship.protect().ship),
                loc);
    }
    if (const auto* mirror = ship.as<SpaceMirrorShip>();
        mirror && mirror->aimed_level() == ScopeLevel::LEVEL_SHIP &&
        mirror->aimed_ship().has_value()) {
      expect_ne(em.peek_ship(*mirror->aimed_ship()), nullptr,
                std::format("Space mirror #{} aims at non-existent ship #{}",
                            ship.number(), *mirror->aimed_ship()),
                loc);
    }
    if (const auto* xport = ship.as<TransporterShip>();
        xport && xport->target_ship().has_value()) {
      expect_ne(em.peek_ship(*xport->target_ship()), nullptr,
                std::format("Transporter #{} targets non-existent ship #{}",
                            ship.number(), *xport->target_ship()),
                loc);
    }
  }

  // 4. Commodities have valid owner existing in RaceList and governor >= 1
  for (const Commod& commod : CommodList::readonly(em)) {
    if (commod.owner.value > 0) {
      expect_ne(em.peek_race(commod.owner), nullptr,
                std::format("Commodity #{} has non-existent owner {}",
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
