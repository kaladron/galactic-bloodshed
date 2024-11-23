// SPDX-License-Identifier: Apache-2.0

module;

import std;

module gblib;

std::vector<Victory> create_victory_list() {
  std::vector<Victory> victories;
  for (const auto& race : races) {
    Victory vic{.racenum = race.Playernum,
                .name = std::string(race.name),
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
