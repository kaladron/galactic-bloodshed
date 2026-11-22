// SPDX-License-Identifier: Apache-2.0

/// \file jettison.cc
/// \brief Functions for jettisoning cargo into deep space.

module commands;

import gb.entities;
import gb.mechanics;
import gb.presentation.ship_ops;
import gb.services;
import std;

namespace GB::commands {
bool jettison(const command_t& argv, GameObj& g) {
  if (argv.size() < 3) {
    g.out << "Jettison what?\n";
    return false;
  }

  std::int64_t requested_amount = 0;
  if (argv.size() > 3) {
    auto parsed = parse_int<std::int64_t>(argv[3]);
    if (!parsed) {
      g.out << "Invalid amount.\n";
      return false;
    }
    requested_amount = *parsed;
  }

  const char commod = first_char(argv[2]);
  bool success = false;

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    const double old_mass = ship_handle->mass();
    auto res = jettison_ship_cargo(*ship_handle, commod, requested_amount,
                                   g.race->mass);
    if (!res) {
      g.out << GB::presentation::render_jettison_error(res.error());
      if (res.error().reason == JettisonErrorReason::InvalidCommodity) {
        return false;
      }
      continue;
    }
    g.entity_manager.propagate_carried_mass_delta(
        *ship_handle, ship_handle->mass() - old_mass);
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
