// SPDX-License-Identifier: Apache-2.0

/// \file toxicity.cc
/// \brief Change threshold in toxicity to build a waste cannister.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool toxicity(const command_t& argv, GameObj& g) {
  int thresh = std::stoi(argv[1]);

  if (thresh > 100 || thresh < 0) {
    g.out << "Illegal value.\n";
    return false;
  }

  std::optional<std::uint32_t> new_val;
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    if (thresh == 0) {
      p.info(g.player()).tox_thresh = std::nullopt;
    } else {
      p.info(g.player()).tox_thresh = static_cast<std::uint32_t>(thresh);
    }
    new_val = p.info(g.player()).tox_thresh;
  });

  g.out << std::format(" New threshold is: {}\n", new_val.value_or(0));
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
