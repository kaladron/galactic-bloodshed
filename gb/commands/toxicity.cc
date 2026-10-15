// SPDX-License-Identifier: Apache-2.0

/// \file toxicity.cc
/// \brief Change threshold in toxicity to build a waste cannister.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {

bool toxicity(const command_t& argv, GameObj& g) {
  auto scanned = scn::scan<int>(argv[1], "{}");
  if (!scanned || scanned->value() > 100 || scanned->value() < 0) {
    g.out << "Illegal value.\n";
    return false;
  }
  const int thresh = scanned->value();

  std::optional<Percentage> new_val;
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    if (thresh == 0) {
      p.info(g.player()).tox_thresh = std::nullopt;
    } else {
      p.info(g.player()).tox_thresh = static_cast<Percentage>(thresh);
    }
    new_val = p.info(g.player()).tox_thresh;
  });

  g.present(GB::presentation::ToxicityResult{
      .threshold = new_val.value_or(0),
  });
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
