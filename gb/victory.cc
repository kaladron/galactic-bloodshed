// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/victory.h"

#include "gb/races.h"

std::vector<Victory> create_victory_list() {
  std::vector<Victory> vicvec;
  for (player_t i = 1; i <= Num_races; i++) {
    Victory vic{.racenum = i,
                .name = std::string(races[i - 1]->name),
                .tech = races[i - 1]->tech,
                .Thing = races[i - 1]->Metamorph,
                .IQ = racess[i - 1]->IQ,
                .rawscore = races[i - 1]->victory_score};
    if (races[i - 1]->God || races[i - 1]->Guest || races[i - 1]->dissolved)
      vic.no_count = true;
    else
      vic.no_count = false;
    vicvec.emplace_back(vic);
  }
  std::sort(vicvec.begin(), vicvec.end());
  return vicvec;
}
