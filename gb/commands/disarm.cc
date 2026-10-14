// SPDX-License-Identifier: Apache-2.0

/// \file disarm.cc
/// \brief Disarm sector military troops back into civilian population.

module;

import gb.entities;
import gb.services;
import gb.mechanics;
import gb.presentation;
import scnlib;
import std;

module commands;

namespace GB::commands {

bool disarm(const command_t& argv, GameObj& g) {
  auto coords_opt = Coordinates::parse(argv[1]);
  if (!coords_opt) {
    g.out << "Bad format for sector.\n";
    return false;
  }

  std::optional<population_t> requested_amount{std::nullopt};
  if (argv.size() >= 3) {
    auto count_res = scn::scan<population_t>(argv[2], "{}");
    if (!count_res || count_res->value() <= 0) {
      g.out << "You must specify a positive number of civs to arm.\n";
      return false;
    }
    requested_amount = count_res->value();
  }

  auto res = disarm_sector_militia(g.entity_manager, g.snum(), g.pnum(),
                                   *coords_opt, requested_amount, g.player());
  if (!res) {
    g.out << GB::presentation::format_militia_error(res.error());
    return false;
  }

  g.present(*res);
  return true;
}

const CommandDescriptor disarm_cmd{
    .name = "disarm",
    .roles = {.no_guests = true, .star_control = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "disarm <sector x,y> [<# of troops>]",
    .description = "Convert military units back to civilian population",
    .handler = &disarm,
};

}  // namespace GB::commands
