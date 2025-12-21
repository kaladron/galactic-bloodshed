// SPDX-License-Identifier: Apache-2.0

module;

import std;

module gblib;

std::vector<Victory> create_victory_list(EntityManager& entity_manager) {
  std::vector<Victory> victories;
  for (auto race_handle : RaceList(entity_manager)) {
    const auto& race = race_handle.read();
    Victory vic{.racenum = race.Playernum,
                .name = race.name,
                .tech = race.tech,
                .Thing = race.Metamorph,
                .IQ = race.IQ,
                .rawscore = race.victory_score};
    if (race.God || race.Guest || race.dissolved) vic.no_count = true;
    victories.emplace_back(vic);
  }
  std::ranges::sort(victories, std::less());

  return victories;
}
