// SPDX-License-Identifier: Apache-2.0

/// \file toxicity.cc
/// \brief Change threshold in toxicity to build a waste cannister.

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void toxicity(const command_t& argv, GameObj& g) {
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

  if (g.level() != ScopeLevel::LEVEL_PLAN) {
    g.out << "scope must be a planet.\n";
    return;
  }
  const auto& star = *g.entity_manager.peek_star(g.snum());
  if (!enufAP(g.entity_manager, g.player(), g.governor(),
              star.AP(g.player() - 1), APcount)) {
    return;
  }

  auto planet_handle = g.entity_manager.get_planet(g.snum(), g.pnum());
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return;
  }
  auto& p = *planet_handle;
  p.info(g.player() - 1).tox_thresh = thresh;
  deductAPs(g, APcount, g.snum());

  g.out << std::format(" New threshold is: {}\n",
                       p.info(g.player() - 1).tox_thresh);
}
}  // namespace GB::commands