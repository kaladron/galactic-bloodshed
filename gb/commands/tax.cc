// SPDX-License-Identifier: Apache-2.0

/// \file tax.cc
/// \brief Query or set planetary tax rate.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;
import scnlib;

module commands;

namespace GB::commands {

bool tax(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();

  if (!g.race->has_government_center()) {
    g.out << "You have no government center active.\n";
    return false;
  }

  if (argv.size() < 2) {
    g.present(GB::presentation::TaxStatusViewModel{
        .planet = GB::presentation::project_planet_view(
            g.entity_manager, g.scope_context(), g.snum(), g.pnum())});
    return true;
  }

  auto parsed_tax = scn::scan<int>(argv[1], "{}");
  if (!parsed_tax || parsed_tax->value() > 100 || parsed_tax->value() < 0) {
    g.out << "Illegal value.\n";
    return false;
  }
  const auto new_tax = static_cast<Percentage>(parsed_tax->value());

  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
    planet.info(Playernum).newtax = new_tax;
  });
  g.present(GB::presentation::make_output_message("Set.\n"));
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