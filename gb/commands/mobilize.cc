// SPDX-License-Identifier: Apache-2.0

/// \file mobilize.cc
/// \brief Persuade people to build military stuff.

/*
 *    Sectors that are mobilized produce Destructive Potential in
 *    proportion to the % they are mobilized.  they are also more
 *    damage-resistant.
 */

module;

import gb.entities;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {

bool mobilize(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  const ap_t APcount = 1;

  if (argv.size() < 2) {
    g.entity_manager.with_planet(g.snum(), g.pnum(), [&](const Planet& planet) {
      g.out << std::format("Current mobilization: {}    Quota: {}\n",
                           planet.info(Playernum).comread,
                           planet.info(Playernum).mob_set);
    });
    return true;
  }

  auto scanned = scn::scan<int>(argv[1], "{}");
  if (!scanned || scanned->value() > 100 || scanned->value() < 0) {
    g.out << "Illegal value.\n";
    return false;
  }
  int sum_mob = scanned->value();

  if (!g.deduct_ap(g.snum(), APcount)) {
    g.out << std::format("You don't have {} action points there.\n", APcount);
    return false;
  }

  g.entity_manager.mutate_planet(g.snum(), g.pnum(), [&](Planet& planet) {
    planet.info(Playernum).mob_set = sum_mob;
  });
  return true;
}

const CommandDescriptor mobilize_cmd{
    .name = "mobilize",
    .roles = {.star_control = true},
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 1,
    .syntax = "mobilize [<percentage>]",
    .description =
        "Set planetary mobilization percentage for military production",
    .handler = &mobilize,
};

}  // namespace GB::commands
