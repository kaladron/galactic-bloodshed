// SPDX-License-Identifier: Apache-2.0

/// \file technology.cc
/// \brief increase investment in technological development

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void technology(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << std::format("scope must be a planet ({}).\n",
                         static_cast<int>(g.level));
    return;
  }

  const auto* star = g.entity_manager.peek_star(g.snum);
  if (!star) {
    g.out << "Star not found.\n";
    return;
  }

  // Check control: governor must match or be 0
  if (Governor != 0 && star->governor[Playernum - 1] != Governor) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  if (!enufAP(Playernum, Governor, star->AP[Playernum - 1], APcount)) {
    return;
  }

  auto planet_handle = g.entity_manager.get_planet(g.snum, g.pnum);
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }

  if (argv.size() < 2) {
    const auto& p = planet_handle.read();
    g.out << std::format(
        "Current investment : {}    Technology production/update: {:.3f}\n",
        p.info[Playernum - 1].tech_invest,
        tech_prod(p.info[Playernum - 1].tech_invest,
                  p.info[Playernum - 1].popn));
    return;
  }

  money_t invest = std::stoi(argv[1]);

  if (invest < 0) {
    g.out << "Illegal value.\n";
    return;
  }

  auto& p = *planet_handle;
  p.info[Playernum - 1].tech_invest = invest;

  deductAPs(g, APcount, g.snum);

  g.out << std::format(
      "   New (ideal) tech production: {:.3f} (this planet)\n",
      tech_prod(p.info[Playernum - 1].tech_invest, p.info[Playernum - 1].popn));
}
}  // namespace GB::commands