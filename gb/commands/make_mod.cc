// SPDX-License-Identifier: Apache-2.0

/// \file make_mod.cc
/// \brief Make and modify command implementations.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool make_mod(const command_t& argv, GameObj& g) {
  const bool is_make = (argv[0] == "make");
  bool ok = false;

  g.entity_manager.mutate_ship(g.shipno(), [&](Ship& dirship) {
    if (!g.check_commandable(dirship)) {
      return;
    }
    if (dirship.type() != ShipType::OTYPE_FACTORY) {
      g.out << "That is not a factory.\n";
      return;
    }
    if (dirship.on() && argv.size() > 1) {
      g.out << "This factory is already online.\n";
      return;
    }
    const auto& race = *g.race;

    if (is_make) {
      if (argv.size() < 2) {
        auto specs = inspect_factory_specs(dirship, race);
        if (!specs) {
          g.out << "No ship type specified.\n";
          return;
        }
        g.present(*specs);
        ok = true;
        return;
      }
      auto res =
          configure_factory_blueprint(dirship, race, g.shipno(), argv[1][0]);
      if (!res) {
        g.out << GB::presentation::format_factory_make_error(res.error());
        return;
      }
      g.present(*res);
      ok = true;
      return;
    }

    auto res = modify_factory_attribute(dirship, race, argv);
    if (!res) {
      g.out << GB::presentation::format_factory_modify_error(res.error());
      return;
    }
    g.present(*res);
    ok = true;
  });
  return ok;
}

const CommandDescriptor make_cmd{
    .name = "make",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::ship_only(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "make [<shiptype>]",
    .description = "Configure ship type to build at a factory installation",
    .handler = &make_mod,
};

const CommandDescriptor modify_cmd{
    .name = "modify",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::ship_only(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "modify <characteristic> [<value>] [<extra>]",
    .description = "Modify ship specifications at a factory installation",
    .handler = &make_mod,
};

}  // namespace GB::commands
