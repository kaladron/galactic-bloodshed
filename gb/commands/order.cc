// SPDX-License-Identifier: Apache-2.0

/// \file order.cc
/// \brief Handler for order command.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import std;
#undef stdout

module commands;

namespace GB::commands {
bool order(const command_t& argv, GameObj& g) {
  player_t playernum = g.player();
  governor_t governor = g.governor();

  g.present(ShipOrdersHeader{});
  if (argv.size() == 1) {
    const ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
    for (const Ship& ship : ships) {
      if (validate_commandable(ship, playernum, governor)) {
        g.present(query_ship_order(g.entity_manager, ship));
      }
    }
    return true;
  }

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    Ship& ship = *ship_handle;

    if (argv.size() > 2) {
      if (auto res =
              give_orders(g.entity_manager, g.scope_context(), argv, ship);
          res) {
        g.present(*res);
      } else {
        g.out << GB::presentation::format_order_error(res.error());
      }
    }

    g.present(query_ship_order(g.entity_manager, ship));
  }
  return true;
}

const CommandDescriptor order_cmd{
    .name = "order",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "order [<ship> [<order> [<args>]]]",
    .description = "Give standing orders to ships or view ship orders",
    .handler = &order,
};

}  // namespace GB::commands
