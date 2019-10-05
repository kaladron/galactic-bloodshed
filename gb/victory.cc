// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import gblib;

#include "gb/victory.h"

import std;

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/races.h"
#include "gb/vars.h"

static bool constexpr victory_sort(const struct vic &a, const struct vic &b) {
  if (a.no_count) return true;
  if (b.no_count) return false;

  if (b.rawscore > a.rawscore) return true;
  if (b.rawscore < a.rawscore) return false;

  // Must be equal
  return true;
}

void victory(const command_t &argv, GameObj &g) {
  /*
  #ifndef VICTORY
  g.out << "Victory conditions disabled.\n";
  return;
  #endif
  */
  int count = (argv.size() > 1) ? std::stoi(argv[1]) : Num_races;
  if (count > Num_races) count = Num_races;

  auto vic = create_victory_list();

  g.out << "----==== PLAYER RANKINGS ====----\n";
  sprintf(buf, "%-4.4s %-15.15s %8s\n", "No.", "Name", (g.god ? "Score" : ""));
  notify(g.player, g.governor, buf);
  for (int i = 0; i < count; i++) {
    if (g.god)
      sprintf(buf, "%2d %c [%2d] %-15.15s %5ld  %6.2f %3d %s %s\n", i + 1,
              vic[i].Thing ? 'M' : ' ', vic[i].racenum, vic[i].name,
              vic[i].rawscore, vic[i].tech, vic[i].IQ,
              races[vic[i].racenum - 1]->password,
              races[vic[i].racenum - 1]->governor[0].password);
    else
      sprintf(buf, "%2d   [%2d] %-15.15s\n", i + 1, vic[i].racenum,
              vic[i].name);
    notify(g.player, g.governor, buf);
  }
}

std::vector<struct vic> create_victory_list() {
  std::vector<struct vic> vicvec;
  for (player_t i = 1; i <= Num_races; i++) {
    struct vic vic;
    vic.racenum = i;
    strcpy(vic.name, races[i - 1]->name);
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
