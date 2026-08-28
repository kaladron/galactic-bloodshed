// SPDX-License-Identifier: Apache-2.0

/// \file dump.cc
/// \brief Transfer exploration data to another player.

module;

import gb.entities;
import gb.services;
import std;
import notification;
import session;

module commands;

namespace GB::commands {

bool dump(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  starnum_t star_id = 0;

  player_t player = get_player(g.entity_manager, argv[1]);
  if (player.value == 0) {
    g.out << "No such player.\n";
    return false;
  }

  /* transfer all planet and star knowledge to the player */
  /* get all stars and planets */
  if (argv.size() < 3) {
    for (auto current_star_handle : StarList(g.entity_manager)) {
      auto& current_star = *current_star_handle;
      star_id = current_star.get_struct().star_id;

      if (isset(current_star.explored(), Playernum)) {
        setbit(current_star.explored(), player);

        for (auto planet_handle :
             PlanetList(g.entity_manager, star_id, current_star)) {
          auto& planet = *planet_handle;
          if (planet.info(Playernum).explored) {
            planet.info(player).explored = 1;
          }
        }
      }
    }
  } else { /* list of places given */
    for (const auto& place_arg : argv | std::views::drop(2)) {
      Place where{g, place_arg, true};
      if (!where.err && where.level != ScopeLevel::LEVEL_UNIV &&
          where.level != ScopeLevel::LEVEL_SHIP) {
        star_id = where.snum;
        g.entity_manager.mutate_star(star_id, [&](Star& current_star) {
          if (isset(current_star.explored(), Playernum)) {
            setbit(current_star.explored(), player);

            for (auto planet_handle :
                 PlanetList(g.entity_manager, star_id, current_star)) {
              auto& planet = *planet_handle;
              if (planet.info(Playernum).explored) {
                planet.info(player).explored = 1;
              }
            }
          }
        });
      }
    }
  }

  warn_race(g.session_registry, g.entity_manager, player,
            std::format("{} [{}] has given you exploration data.\n",
                        g.race->name, Playernum));
  g.out << "Exploration Data transferred.\n";
  return true;
}

const CommandDescriptor dump_cmd{
    .name = "dump",
    .roles =
        {
            .no_guests = true,
            .leader_only = true,
        },
    .scopes = AllowedScopes::any(),
    .ap = APCost::fixed_star(10),
    .min_args = 2,
    .syntax = "dump <player> [<place> ...]",
    .description =
        "Transfer exploration data about stars/planets to another player",
    .handler = &dump,
};

}  // namespace GB::commands
