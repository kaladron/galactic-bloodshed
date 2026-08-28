// SPDX-License-Identifier: Apache-2.0

/// \file tax.cc
/// \brief Query or set planetary tax rate.

module;

import gb.entities;
import gb.services;
import std;
import scnlib;

module commands;

namespace GB::commands {

bool tax(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();

  if (g.race->Gov_ship == 0) {
    g.out << "You have no government center active.\n";
    return false;
  }

  if (argv.size() < 2) {
    const auto& planet = *g.entity_manager.peek_planet(g.snum(), g.pnum());
    g.out << std::format("Current tax rate: {}%    Target: {}%\n",
                         planet.info(Playernum).tax,
                         planet.info(Playernum).newtax);
    return true;
  }

  auto parsed_tax = scn::scan<int>(argv[1], "{}");
  if (!parsed_tax || parsed_tax->value() > 100 || parsed_tax->value() < 0) {
    g.out << "Illegal value.\n";
    return false;
  }

  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
    planet.info(Playernum).newtax = parsed_tax->value();
  });
  g.out << "Set.\n";
  return true;
}

const CommandDescriptor tax_cmd{
    .name = "tax",
    .roles = {.no_guests = true, .star_control = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "tax [<rate>]",
    .description = "Query or set planetary tax rate",
    .handler = &tax,
};

}  // namespace GB::commands