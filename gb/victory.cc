// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std.compat;

#include "gb/victory.h"

#include "gb/races.h"

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
  std::sort(victories.begin(), victories.end());
  return victories;
}
