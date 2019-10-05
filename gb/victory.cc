// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;
import std;

#include "gb/victory.h"

#include "gb/races.h"

namespace {
bool constexpr victory_sort(const struct vic &a, const struct vic &b) {
  if (a.no_count) return true;
  if (b.no_count) return false;

  if (b.rawscore > a.rawscore) return true;
  if (b.rawscore < a.rawscore) return false;

  // Must be equal
  return true;
}
}  // namespace

std::vector<vic> create_victory_list() {
  std::vector<vic> vicvec;
  for (player_t i = 1; i <= Num_races; i++) {
    struct vic vic;
    vic.racenum = i;
    vic.name = std::string(races[i - 1]->name);
    vic.rawscore = races[i - 1]->victory_score;
    vic.tech = races[i - 1]->tech;
    vic.Thing = races[i - 1]->Metamorph;
    vic.IQ = races[i - 1]->IQ;
    if (races[i - 1]->God || races[i - 1]->Guest || races[i - 1]->dissolved)
      vic.no_count = true;
    else
      vic.no_count = false;
    vicvec.emplace_back(vic);
  }
  std::sort(vicvec.begin(), vicvec.end(), victory_sort);
  return vicvec;
}
