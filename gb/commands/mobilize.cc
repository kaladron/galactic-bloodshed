// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/// \file mobilize.c
/// \brief Persuade people to build military stuff.

/*
 *    Sectors that are mobilized produce Destructive Potential in
 *    proportion to the % they are mobilized.  they are also more
 *    damage-resistant.
 */

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void mobilize(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "scope must be a planet.\n";
    return;
  }
  const auto* star = g.entity_manager.peek_star(g.snum);
  if (!star) {
    g.out << "Star not found.\n";
    return;
  }
  if (!star->control(Playernum, Governor)) {
    g.out << "You are not authorized to do this here.\n";
    return;
  }
  if (!enufAP(Playernum, Governor, star->AP(Playernum - 1), APcount)) {
    return;
  }

  auto planet = g.entity_manager.get_planet(g.snum, g.pnum);
  if (!planet.get()) {
    g.out << "Planet not found.\n";
    return;
  }

  if (argv.size() < 2) {
    notify(Playernum, Governor,
           std::format("Current mobilization: {}    Quota: {}\n",
                       planet->info(Playernum - 1).comread,
                       planet->info(Playernum - 1).mob_set));
    return;
  }
  int sum_mob = std::stoi(argv[1]);

  if (sum_mob > 100 || sum_mob < 0) {
    g.out << "Illegal value.\n";
    return;
  }
  planet->info(Playernum - 1).mob_set = sum_mob;
  deductAPs(g, APcount, g.snum);
}
}  // namespace GB::commands
