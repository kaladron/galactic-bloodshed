// SPDX-License-Identifier: Apache-2.0

/// \file modify.cc
/// \brief Factory ship blueprint attribute modification command.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool modify(const command_t& argv, GameObj& g) {
  const Ship& ship = *g.entity_manager.peek_ship(g.shipno());
  if (!g.check_commandable(ship)) {
    return false;
  }
  const auto* fac_view = ship.as<FactoryShip>();
  if (!fac_view) {
    g.out << "That is not a factory.\n";
    return false;
  }
  if (fac_view->on()) {
    g.out << "This factory is already online.\n";
    return false;
  }

  bool ok = false;
  g.entity_manager.mutate_ship(g.shipno(), [&](Ship& dirship) {
    auto res =
        modify_factory_attribute(*dirship.as<FactoryShip>(), *g.race, argv);
    if (!res) {
      g.out << GB::presentation::format_factory_modify_error(res.error());
      return;
    }
    g.present(*res);
    ok = true;
  });
  return ok;
}

const CommandDescriptor modify_cmd{
    .name = "modify",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::ship_only(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "modify <characteristic> [<value>] [<extra>]",
    .description = "Modify ship specifications at a factory installation",
    .handler = &modify,
};

}  // namespace GB::commands
