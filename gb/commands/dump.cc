// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;
import notification;
import session;

module commands;

namespace GB::commands {
void dump(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();
  ap_t APcount = 10;
  player_t player;
  int star_id;

  const auto* star = g.entity_manager.peek_star(g.snum());
  if (!enufAP(Playernum, Governor, star->AP(Playernum - 1), APcount)) return;

  if (!(player = get_player(g.entity_manager, argv[1]))) {
    g.out << "No such player.\n";
    return;
  }

  /* transfer all planet and star knowledge to the player */
  /* get all stars and planets */
  if (g.race->Guest) {
    g.out << "Cheater!\n";
    return;
  }
  if (Governor) {
    g.out << "Only leaders are allowed to use dump.\n";
    return;
  }
  const auto& sdata = *g.entity_manager.peek_universe();

  if (argv.size() < 3) {
    for (auto current_star_handle : StarList(g.entity_manager)) {
      auto& current_star = *current_star_handle;
      star_id = current_star.get_struct().star_id;

      if (isset(current_star.explored(), Playernum)) {
        setbit(current_star.explored(), player);

        for (auto planet_handle :
             PlanetList(g.entity_manager, star_id, current_star)) {
          auto& planet = *planet_handle;
          if (planet.info(Playernum - 1).explored) {
            planet.info(player - 1).explored = 1;
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
        auto current_star_handle = g.entity_manager.get_star(star_id);
        auto& current_star = *current_star_handle;

        if (isset(current_star.explored(), Playernum)) {
          setbit(current_star.explored(), player);

          for (auto planet_handle :
               PlanetList(g.entity_manager, star_id, current_star)) {
            auto& planet = *planet_handle;
            if (planet.info(Playernum - 1).explored) {
              planet.info(player - 1).explored = 1;
            }
          }
        }
      }
    }
  }

  deductAPs(g, APcount, g.snum());

  warn_race(get_session_registry(g), g.entity_manager, player,
            std::format("{} [{}] has given you exploration data.\n",
                        g.race->name, Playernum));
  g.out << "Exploration Data transferred.\n";
}
}  // namespace GB::commands
