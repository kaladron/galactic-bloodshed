// SPDX-License-Identifier: Apache-2.0

/// \file order.cc
/// \brief Handler for order command.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation.navigation;
import gb.presentation.ship_ops;
import gb.services;
import std;

module commands;

namespace GB::commands {
bool order(const command_t& argv, GameObj& g) {
  g.present(ShipOrdersHeader{});
  if (argv.size() <= 2) {
    const std::string_view filter =
        (argv.size() == 1) ? std::string_view{"*"} : std::string_view{argv[1]};
    GB::presentation::SightContext sight;
    for (const Ship& ship : ScopedCommandableShips::readonly(g, filter)) {
      g.present(query_ship_order(g.entity_manager, ship));
      if (g.ui_mode() == UiMode::JSON) {
        if (auto sv = GB::presentation::project_ship_view(
                g.entity_manager, g.scope_context(), ship, &sight)) {
          g.out << GB::presentation::render_json_ship_view(*sv);
        }
      }
    }
    return true;
  }

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    Ship& ship = *ship_handle;
    if (auto res = give_orders(g.entity_manager, g.scope_context(), argv, ship);
        res) {
      g.present(*res);
    } else {
      g.out << GB::presentation::format_order_error(res.error());
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
