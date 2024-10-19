// SPDX-License-Identifier: Apache-2.0

/// \file toxicity.cc
/// \brief Change threshold in toxicity to build a waste cannister.

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void toxicity(const command_t &argv, GameObj &g) {
  constexpr ap_t APcount = 1;

  if (argv.size() != 2) {
    g.out << "Provide exactly one value between 0 and 100.\n";
    return;
  }

  int thresh = std::stoi(argv[1]);

  if (thresh > 100 || thresh < 0) {
    g.out << "Illegal value.\n";
    return;
  }

  if (g.level != ScopeLevel::LEVEL_PLAN) {
    g.out << "scope must be a planet.\n";
    return;
  }
  if (!enufAP(g.player, g.governor, stars[g.snum].AP(g.player - 1), APcount)) {
    return;
  }

  auto p = getplanet(g.snum, g.pnum);
  p.info[g.player - 1].tox_thresh = thresh;
  putplanet(p, stars[g.snum], g.pnum);
  deductAPs(g, APcount, g.snum);

  g.out << std::format(" New threshold is: {}\n",
                       p.info[g.player - 1].tox_thresh);
}
}  // namespace GB::commands