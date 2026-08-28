// SPDX-License-Identifier: Apache-2.0

/// \file technology.cc
/// \brief Query or set planetary technology investment.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool technology(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();

  if (argv.size() < 2) {
    g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& planet) {
      g.out << std::format(
          "Current investment : {}    Technology production/update: "
          "{:.3f}\n",
          planet.info(Playernum).tech_invest,
          tech_prod(planet.info(Playernum).tech_invest,
                    planet.info(Playernum).popn));
    });
    return true;
  }

  money_t invest = 0;
  try {
    invest = std::stoi(argv[1]);
  } catch (...) {
    g.out << "Illegal value.\n";
    return false;
  }

  if (invest < 0) {
    g.out << "Illegal value.\n";
    return false;
  }

  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    p.info(Playernum).tech_invest = invest;

    g.out << std::format(
        "   New (ideal) tech production: {:.3f} (this planet)\n",
        tech_prod(p.info(Playernum).tech_invest, p.info(Playernum).popn));
  });
  return true;
}

const CommandDescriptor technology_cmd{
    .name = "technology",
    .roles = {.star_control = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::fixed_star(1),
    .min_args = 1,
    .syntax = "technology [<investment>]",
    .description = "Query or set planetary technology investment",
    .handler = &technology,
};

}  // namespace GB::commands
