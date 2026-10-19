// SPDX-License-Identifier: Apache-2.0

/// \file land.cc
/// \brief Land a ship on a planet or friendly mothership.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation.combat;
import gb.presentation.navigation;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool land(const command_t& argv, GameObj& g) {
  bool any_landed = false;

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    auto res =
        land_single_ship(g.entity_manager, *ship_handle, argv[2], g.player(),
                         g.governor(), g.snum(), g.pnum(), g.god());
    if (!res) {
      g.out << GB::presentation::format_land_error(res.error());
      continue;
    }

    for (const auto& salvo : res->defense_salvos) {
      const std::string p_short =
          GB::presentation::render_ship_shot_short(salvo.shot);
      const std::string p_long =
          GB::presentation::render_ship_shot_long(salvo.shot);
      post(g.entity_manager, p_short, NewsType::COMBAT);
      notify_star(g.session_registry, g.entity_manager, 0, 0, res->star_id,
                  p_short);
      warn_player(g.session_registry, g.entity_manager, salvo.defender_player,
                  salvo.defender_governor, p_long);
    }

    if (const auto obs_msg =
            GB::presentation::render_land_observer_telegram(*res)) {
      for (const auto& obs : res->observers) {
        warn_player(g.session_registry, g.entity_manager, obs.player,
                    obs.governor, *obs_msg);
      }
    }

    g.present(*res);
    any_landed = true;
  }

  return any_landed;
}

const CommandDescriptor land_cmd{
    .name = "land",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "land <ship> <#mothership | x,y>",
    .description = "Land a ship onto a planet sector or mothership",
    .handler = &land,
};

}  // namespace GB::commands
