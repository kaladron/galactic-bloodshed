// SPDX-License-Identifier: Apache-2.0

/// \file test.cc
/// \brief Implementations of test framework helpers and verification routines.

module;

#include <cassert>

module test;

import commands;
import dallib;
import gblib;
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

SessionRegistry& get_test_session_registry() {
  return get_null_session_registry();
}

std::vector<SessionInfo>
RecordingSessionRegistry::get_connected_sessions() const {
  return sessions;
}

bool RecordingSessionRegistry::is_connected(player_t player,
                                            governor_t gov) const {
  return std::ranges::any_of(sessions, [&](const auto& s) {
    return s.player == player && s.governor == gov && s.connected;
  });
}

void RecordingSessionRegistry::notify_race(player_t race,
                                           const std::string& message) {
  notifications.push_back({
      .player = race,
      .governor = 0,
      .message = message,
      .is_broadcast = true,
  });
}

bool RecordingSessionRegistry::notify_player(player_t race, governor_t gov,
                                             const std::string& message) {
  notifications.push_back({
      .player = race,
      .governor = gov,
      .message = message,
      .is_broadcast = false,
  });
  return true;
}

bool RecordingSessionRegistry::update_in_progress() const {
  return update_in_progress_flag;
}

void RecordingSessionRegistry::set_update_in_progress(bool val) {
  update_in_progress_flag = val;
}

bool RecordingSessionRegistry::has_received(player_t player,
                                            std::string_view needle) const {
  return std::ranges::any_of(notifications, [&](const auto& n) {
    return n.player == player && n.message.contains(needle);
  });
}

bool RecordingSessionRegistry::has_broadcast(std::string_view needle) const {
  return std::ranges::any_of(notifications, [&](const auto& n) {
    return n.is_broadcast && n.message.contains(needle);
  });
}

std::vector<std::string>
RecordingSessionRegistry::messages_for(player_t player) const {
  std::vector<std::string> msgs;
  for (const auto& n : notifications) {
    if (n.player == player) {
      msgs.push_back(n.message);
    }
  }
  return msgs;
}

void RecordingSessionRegistry::clear_notifications() {
  notifications.clear();
}
