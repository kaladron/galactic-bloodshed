// SPDX-License-Identifier: Apache-2.0

/// \file order.cc
/// \brief Handler for order command.

module;

import gb.entities;
import gb.services;
import std;
#undef stdout

module commands;

namespace GB::commands {
bool order(const command_t& argv, GameObj& g) {
  player_t playernum = g.player();
  governor_t governor = g.governor();
  ap_t ap_count = 1;

  if (argv.size() == 1) { /* display all ship orders */
    display_orders_header(g);
    const ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
    for (const Ship& ship : ships) {
      if (ship.owner() == playernum && authorized(governor, ship)) {
        display_orders(g, ship);
      }
    }
    return true;
  } else if (argv.size() >= 2) {
    display_orders_header(g);
    ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
    for (auto ship_handle : ships) {
      Ship& ship = *ship_handle;

      if (!ship_matches_filter(argv[1], ship)) continue;
      if (!authorized(governor, ship)) continue;

      if (argv.size() > 2) {
        give_orders(g, argv, ap_count, ship);
      }

      display_orders(g, ship);

      // Early exit for specific ship number filters
      if (is_ship_number_filter(argv[1])) break;
    }
    return true;
  } else {
    g.out << "I don't understand what you mean.\n";
    return false;
  }
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
