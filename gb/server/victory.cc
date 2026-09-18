// SPDX-License-Identifier: Apache-2.0

/// \file victory.cc
/// \brief Victory condition calculation and victory rankings generation.

module;

import std;

module gblib;

std::vector<Victory> create_victory_list(EntityManager& entity_manager) {
  std::vector<Victory> victories;
  for (const Race& race : RaceList::readonly(entity_manager)) {
    Victory vic{.racenum = race.Playernum,
                .name = race.name,
                .tech = race.tech,
                .thing = race.Metamorph,
                .iq = race.IQ,
                .rawscore = race.victory_score};
    if (race.God || race.Guest || race.dissolved) vic.no_count = true;
    victories.emplace_back(vic);
  }
  std::ranges::sort(victories, std::less());

  return victories;
}
