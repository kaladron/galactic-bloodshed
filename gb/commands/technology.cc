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
  if (!control(stars[g.snum], Playernum, Governor)) {
    g.out << "You are not authorized to do that here.\n";
    return;
  }
  if (!enufAP(Playernum, Governor, stars[g.snum].AP[Playernum - 1], APcount)) {
    return;
  }

  auto p = getplanet(g.snum, g.pnum);

  if (argv.size() < 2) {
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

  p.info[Playernum - 1].tech_invest = invest;

  putplanet(p, stars[g.snum], g.pnum);

  deductAPs(g, APcount, g.snum);

  g.out << std::format(
      "   New (ideal) tech production: {:.3f} (this planet)\n",
      tech_prod(p.info[Playernum - 1].tech_invest, p.info[Playernum - 1].popn));
}
}  // namespace GB::commands