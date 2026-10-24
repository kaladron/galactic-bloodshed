// SPDX-License-Identifier: Apache-2.0

/// \file technology.cc
/// \brief Query or set planetary technology investment.

module;

import gb.entities;
import gb.presentation.planet_views;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {

bool technology(const command_t& argv, GameObj& g) {
  if (argv.size() > 2) {
    g.out << "Syntax: technology [<investment>]\n";
    return false;
  }

  const player_t player = g.player();
  if (argv.size() == 1) {
    const auto& planet = *g.entity_manager.peek_planet(g.snum(), g.pnum());
    const auto& info = planet.info(player);
    g.present(GB::presentation::TechnologyViewModel{
        .updated = false,
        .investment = info.tech_invest,
        .tech_production = tech_prod(info.tech_invest, info.popn),
    });
    return true;
  }

  auto scan_res = scn::scan<money_t>(argv[1], "{}");
  if (!scan_res || !scan_res->range().empty() || scan_res->value() < 0) {
    g.out << "Illegal value.\n";
    return false;
  }
  const money_t invest = scan_res->value();

  double new_prod = 0.0;
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    auto& info = p.info(player);
    info.tech_invest = invest;
    new_prod = tech_prod(info.tech_invest, info.popn);
  });
  g.present(GB::presentation::TechnologyViewModel{
      .updated = true,
      .investment = invest,
      .tech_production = new_prod,
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
