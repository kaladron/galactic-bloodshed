// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "gb/victory.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "gb/GB_server.h"
#include "gb/buffers.h"
#include "gb/races.h"
#include "gb/vars.h"

static auto constexpr victory_sort(const void *A, const void *B) {
  const auto *a = (const struct vic *)A;
  const auto *b = (const struct vic *)B;
  if (a->no_count) return 1;
  if (b->no_count) return -1;

  if (b->rawscore > a->rawscore) return 1;
  if (b->rawscore < a->rawscore) return -1;

  // Must be equal
  return 0;
}

void victory(const command_t &argv, GameObj &g) {
  struct vic vic[MAXPLAYERS];

  /*
  #ifndef VICTORY
  g.out << "Victory conditions disabled.\n";
  return;
  #endif
  */
  int count = (argv.size() > 1) ? std::stoi(argv[1]) : Num_races;
  if (count > Num_races) count = Num_races;

  create_victory_list(vic);

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

void create_victory_list(struct vic vic[MAXPLAYERS]) {
  Race *vic_races[MAXPLAYERS];

  for (player_t i = 1; i <= Num_races; i++) {
    vic_races[i - 1] = races[i - 1];
    vic[i - 1].no_count = 0;
  }
  for (player_t i = 1; i <= Num_races; i++) {
    vic[i - 1].racenum = i;
    strcpy(vic[i - 1].name, vic_races[i - 1]->name);
    vic[i - 1].rawscore = vic_races[i - 1]->victory_score;
    /*    vic[i-1].rawscore = vic_races[i-1]->morale; */
    vic[i - 1].tech = vic_races[i - 1]->tech;
    vic[i - 1].Thing = vic_races[i - 1]->Metamorph;
    vic[i - 1].IQ = vic_races[i - 1]->IQ;
    if (vic_races[i - 1]->God || vic_races[i - 1]->Guest ||
        vic_races[i - 1]->dissolved)
      vic[i - 1].no_count = 1;
  }
  qsort(vic, Num_races, sizeof(struct vic), victory_sort);
}