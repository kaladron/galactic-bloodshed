// SPDX-License-Identifier: Apache-2.0

/// \file toxicity.cc
/// \brief Change threshold in toxicity to build a waste cannister.

module;

import gblib;
import std;

module commands;

namespace GB::commands {

bool toxicity(const command_t& argv, GameObj& g) {
  int thresh = std::stoi(argv[1]);

  if (thresh > 100 || thresh < 0) {
    g.out << "Illegal value.\n";
    return false;
  }

  auto planet_handle = g.entity_manager.get_planet(g.snum(), g.pnum());
  if (!planet_handle.get()) {
    g.out << "Planet not found.\n";
    return false;
  }
  auto& p = *planet_handle;
  p.info(g.player()).tox_thresh = thresh;

  g.out << std::format(" New threshold is: {}\n",
                       p.info(g.player()).tox_thresh);
  return true;
}

const CommandDescriptor toxicity_cmd{
    .name = "toxicity",
    .roles = {},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::fixed_star(1),
    .min_args = 2,
    .syntax = "toxicity <threshold>",
    .description =
        "Set planetary toxicity threshold for waste cannister construction",
    .handler = &toxicity,
};

}  // namespace GB::commands
