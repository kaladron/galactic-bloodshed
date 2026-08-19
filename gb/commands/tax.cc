// SPDX-License-Identifier: Apache-2.0

/// \file tax.cc
/// \brief Query or set planetary tax rate.

module;

import gblib;
import std;

module commands;

namespace GB::commands {

bool tax(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();

  if (g.race->Gov_ship == 0) {
    g.out << "You have no government center active.\n";
    return false;
  }

  auto planet_handle = g.entity_manager.get_planet(g.snum(), g.pnum());
  auto& planet = *planet_handle;

  if (argv.size() < 2) {
    g.out << std::format("Current tax rate: {}%    Target: {}%\n",
                         planet.info(Playernum).tax,
                         planet.info(Playernum).newtax);
    return true;
  }

  int sum_tax = 0;
  try {
    sum_tax = std::stoi(argv[1]);
  } catch (...) {
    g.out << "Illegal value.\n";
    return false;
  }

  if (sum_tax > 100 || sum_tax < 0) {
    g.out << "Illegal value.\n";
    return false;
  }
  planet.info(Playernum).newtax = sum_tax;
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