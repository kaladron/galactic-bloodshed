// SPDX-License-Identifier: Apache-2.0

/// \file technology.cc
/// \brief Query or set planetary technology investment.

module;

import gb.entities;
import gb.presentation.planet_views;
import gb.services;
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
        .investment = info.tech_invest,
        .tech_production = tech_prod(info.tech_invest, info.popn),
        .planet = GB::presentation::project_planet_view(
            g.entity_manager, g.scope_context(), g.snum(), g.pnum()),
    });
    return true;
  }

  auto scan_res = parse_int<money_t>(argv[1]);
  if (!scan_res || *scan_res < 0) {
    g.out << "Illegal value.\n";
    return false;
  }
  const money_t invest = *scan_res;

  constexpr ap_t tech_ap_cost = 1;
  if (!g.deduct_ap(g.snum(), tech_ap_cost)) {
    g.out << std::format("You don't have {} action points there.\n",
                         tech_ap_cost);
    return false;
  }

  double new_prod = 0.0;
  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& p) {
    auto& info = p.info(player);
    info.tech_invest = invest;
    new_prod = tech_prod(info.tech_invest, info.popn);
  });
  g.present(GB::presentation::make_output_message(std::format(
      "   New (ideal) tech production: {:.3f} (this planet)\n", new_prod)));
  return true;
}

const CommandDescriptor technology_cmd{
    .name = "technology",
    .roles = {.star_control = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 1,
    .syntax = "technology [<investment>]",
    .description = "Query or set planetary technology investment",
    .handler = &technology,
};

}  // namespace GB::commands
