// SPDX-License-Identifier: Apache-2.0

/// \file jettison.cc
/// \brief Functions for jettisoning cargo into deep space.

module commands;

import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import scnlib;
import std;

namespace GB::commands {
bool jettison(const command_t& argv, GameObj& g) {
  if (argv.size() < 3) {
    g.out << "Jettison what?\n";
    return false;
  }

  int requested_amount = 0;
  if (argv.size() > 3) {
    auto parsed = scn::scan<int>(argv[3], "{}");
    if (!parsed) {
      g.out << "Invalid amount.\n";
      return false;
    }
    requested_amount = parsed->value();
  }

  const char commod = argv[2].empty() ? '\0' : argv[2][0];
  bool success = false;

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    auto res = jettison_ship_cargo(*ship_handle, commod, requested_amount,
                                   g.race->mass);
    if (!res) {
      g.out << GB::presentation::render_jettison_error(res.error());
      if (res.error().reason == JettisonErrorReason::InvalidCommodity) {
        return false;
      }
      continue;
    }
    g.present(*res);
    success = true;
  }
  return success;
}

const CommandDescriptor jettison_cmd{
    .name = "jettison",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "jettison <ship> <commodity> [<amount>]",
    .description = "Unload commodities from a ship into space",
    .handler = &jettison,
};

}  // namespace GB::commands
