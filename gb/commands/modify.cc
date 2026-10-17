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
  bool ok = false;

  g.entity_manager.mutate_ship(g.shipno(), [&](Ship& dirship) {
    if (!g.check_commandable(dirship)) {
      return;
    }
    if (dirship.type() != ShipType::OTYPE_FACTORY) {
      g.out << "That is not a factory.\n";
      return;
    }
    if (dirship.on()) {
      g.out << "This factory is already online.\n";
      return;
    }

    auto res = modify_factory_attribute(dirship, *g.race, argv);
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
