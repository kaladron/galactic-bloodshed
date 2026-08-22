// SPDX-License-Identifier: Apache-2.0

/// \file mobilize.cc
/// \brief Persuade people to build military stuff.

/*
 *    Sectors that are mobilized produce Destructive Potential in
 *    proportion to the % they are mobilized.  they are also more
 *    damage-resistant.
 */

module;

import gblib;
import std;

module commands;

namespace GB::commands {

bool mobilize(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  ap_t APcount = 1;

  auto planet = g.entity_manager.get_planet(g.snum(), g.pnum());
  if (!planet.get()) {
    g.out << "Planet not found.\n";
    return false;
  }

  if (argv.size() < 2) {
    g.out << std::format("Current mobilization: {}    Quota: {}\n",
                         planet->info(Playernum).comread,
                         planet->info(Playernum).mob_set);
    return true;
  }
  int sum_mob = std::stoi(argv[1]);

  if (sum_mob > 100 || sum_mob < 0) {
    g.out << "Illegal value.\n";
    return false;
  }

  if (!g.deduct_ap(g.snum(), APcount)) {
    g.out << std::format("You don't have {} action points there.\n", APcount);
    return false;
  }

  planet->info(Playernum).mob_set = sum_mob;
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
