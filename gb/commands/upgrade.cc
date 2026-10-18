// SPDX-License-Identifier: Apache-2.0

/// \file upgrade.cc
/// \brief Upgrade ship characteristics and systems.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation.construction;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool upgrade(const command_t& argv, GameObj& g) {
  if (g.level() != ScopeLevel::LEVEL_SHIP) {
    g.out << "You have to change scope to the ship you wish to upgrade.\n";
    return false;
  }
  bool ok = false;
  g.entity_manager.mutate_ship(g.shipno(), [&](Ship& dirship) {
    if (!g.check_commandable(dirship)) {
      return;
    }
    const auto res =
        upgrade_single_ship(g.entity_manager, dirship, *g.race, argv);
    if (!res) {
      g.out << GB::presentation::format_upgrade_error(res.error());
      return;
    }
    g.present(*res);
    ok = true;
  });
  return ok;
}

const CommandDescriptor upgrade_cmd{
    .name = "upgrade",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::ship_only(),
    .ap = APCost::fixed_star(1),
    .min_args = 2,
    .syntax = "upgrade <characteristic> [<value>]",
    .description = "Upgrade ship characteristics and systems",
    .handler = &upgrade,
};

}  // namespace GB::commands