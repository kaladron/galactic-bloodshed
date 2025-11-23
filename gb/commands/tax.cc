// SPDX-License-Identifier: Apache-2.0

/// \file tax.cc

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void tax(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 0;

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
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  const auto* race = g.entity_manager.peek_race(Playernum);
  if (!race) {
    g.out << "Race not found.\n";
    return;
  }
  if (!race->Gov_ship) {
    g.out << "You have no government center active.\n";
    return;
  }
  if (race->Guest) {
    g.out << "Sorry, but you can't do this when you are a guest.\n";
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
    g.out << std::format("Current tax rate: {}%    Target: {}%\n",
                         planet->info(Playernum - 1).tax,
                         planet->info(Playernum - 1).newtax);
    return;
  }

  int sum_tax = std::stoi(argv[1]);

  if (sum_tax > 100 || sum_tax < 0) {
    g.out << "Illegal value.\n";
    return;
  }
  planet->info(Playernum - 1).newtax = sum_tax;
  // Auto-saves when planet goes out of scope

  deductAPs(g, APcount, g.snum);
  g.out << "Set.\n";
}
}  // namespace GB::commands