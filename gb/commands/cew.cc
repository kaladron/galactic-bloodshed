// SPDX-License-Identifier: Apache-2.0

/// \file cew.cc
/// \brief Fire Confined Energy Weapons (CEWs) at target ship.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool cew(const command_t& argv, GameObj& g) {
  if (argv.size() < 3) {
    g.out << "Syntax: 'cew <ship> <target>'.\n";
    return false;
  }

  const auto toshiptmp = string_to_shipnum(argv[2]);
  if (!toshiptmp || *toshiptmp <= 0) {
    g.out << "Bad ship number.\n";
    return false;
  }
  const shipnum_t toship = *toshiptmp;

  bool any_fired = false;
  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    auto res = cew_single_ship(g.entity_manager, *ship_handle, toship,
                               g.player(), g.god());
    if (!res) {
      g.out << GB::presentation::render_fire_error(res.error());
      if (res.error().abort_loop) {
        return any_fired;
      }
      continue;
    }

    notify_ship_combat_exchange(g, res->exchange, res->exchange.target_owner,
                                res->exchange.target_governor);
    g.present(*res);
    any_fired = true;
  }

  return any_fired;
}

const CommandDescriptor cew_cmd{
    .name = "cew",
    .roles =
        {
            .no_guests = true,
        },
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "cew <ship> <target>",
    .description = "Fire Confined Energy Weapons (CEWs) at target ship",
    .handler = &cew,
};

}  // namespace GB::commands
