// SPDX-License-Identifier: Apache-2.0

/// \file detonate.cc
/// \brief Detonate space mine(s) command.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool detonate(const command_t& argv, GameObj& g) {
  bool any_detonated = false;

  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    Ship& s = *ship_handle;
    auto report = detonate_ship_mine(g.entity_manager, s);
    if (!report) {
      g.out << GB::presentation::render_detonate_error(report.error());
      continue;
    }

    const std::string notice =
        GB::presentation::render_mine_detonation_notice(*report);
    post(g.entity_manager, notice, NewsType::COMBAT);
    telegram_star(g.entity_manager, s.storbits(), s.owner(), s.governor(),
                  notice);
    for (const auto& victim : report->ship_victims) {
      post(g.entity_manager,
           GB::presentation::render_ship_shot_short(victim.shot),
           NewsType::COMBAT);
      push_telegram(g.entity_manager, victim.victim_owner,
                    victim.victim_governor,
                    GB::presentation::render_ship_shot_long(victim.shot));
    }
    if (report->planet_strike) {
      const std::string planet_msg =
          GB::presentation::render_mine_planet_strike_telegram(*report);
      const auto& star = *g.entity_manager.peek_star(s.storbits());
      for (player_t i : report->planet_strike->nuked_players) {
        push_telegram(g.entity_manager, i, star.governor(i), planet_msg);
      }
      push_telegram(g.entity_manager, s.owner(), s.governor(), planet_msg);
    }
    g.present(*report);
    any_detonated = true;
  }

  return any_detonated;
}

const CommandDescriptor detonate_cmd{
    .name = "detonate",
    .roles =
        {
            .no_guests = true,
        },
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "detonate <mine>",
    .description = "Detonate space mine(s)",
    .handler = &detonate,
};

}  // namespace GB::commands
