// SPDX-License-Identifier: Apache-2.0

/// \file who.cc
/// \brief List currently connected players.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool who(const command_t&, GameObj& g) {
  const std::time_t now = std::time(nullptr);
  const bool is_god = g.god();

  GB::presentation::WhoViewModel vm{
      .now = now,
      .show_cowards = SHOW_COWARDS,
  };

  // Get all connected sessions as metadata (no Session type exposure)
  for (const auto& info : g.session_registry.get_connected_sessions()) {
    if (info.god) continue;  // Skip god sessions

    g.entity_manager.with_race(info.player, [&](const Race& r) {
      const auto& gov = r.governor(info.governor);
      // Check if this player should be visible
      const bool is_visible =
          !gov.toggle.invisible || info.player == g.player() || is_god;

      if (is_visible) {
        std::string star_name;
        if (is_god) {
          g.entity_manager.with_star(info.snum, [&](const Star& star) {
            star_name = star.get_name();
          });
        }
        vm.rows.push_back(GB::presentation::WhoRow{
            .race_name = r.name,
            .governor_name = gov.name,
            .player = info.player,
            .governor = info.governor,
            .idle_seconds = now - info.last_time,
            .star_name = std::move(star_name),
            .gag = gov.toggle.gag,
            .invisible = gov.toggle.invisible,
        });
      } else {
        vm.coward_count++;
      }
    });
  }

  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
  return true;
}

const CommandDescriptor who_cmd{
    .name = "who",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "who",
    .description = "List currently connected players",
    .handler = &who,
};

}  // namespace GB::commands
