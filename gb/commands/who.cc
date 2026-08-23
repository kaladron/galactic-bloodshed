// SPDX-License-Identifier: Apache-2.0

/// \file who.cc
/// \brief List currently connected players.

module;

import gblib;
import tabulate;
import std;
#undef stdout

module commands;

namespace GB::commands {

bool who(const command_t&, GameObj& g) {
  std::time_t now = std::time(nullptr);
  bool is_god = g.god();
  int coward_count = 0;

  g.out << std::format("Current Players: {}", std::ctime(&now));

  tabulate::Table table;
  table.add_row({"Race", "Governor", "Player", "Idle", "Star", "Flags"});

  // Get all connected sessions as metadata (no Session type exposure)
  for (const auto& info : g.session_registry.get_connected_sessions()) {
    if (info.god) continue;  // Skip god sessions

    const auto* r = g.entity_manager.peek_race(info.player);
    if (!r) continue;

    // Check if this player should be visible
    bool is_visible = !r->governor[info.governor.value].toggle.invisible ||
                      info.player == g.player() || is_god;

    if (is_visible) {
      std::string gov_name =
          std::format("\"{}\"", r->governor[info.governor.value].name);
      std::string star_name;
      if (is_god) {
        try {
          const auto* star = g.entity_manager.peek_star(info.snum);
          if (star) star_name = star->get_name();
        } catch (const EntityNotFoundError&) {
        }
      }
      std::time_t idle_seconds = now - info.last_time;
      std::string player_gov =
          std::format("[{},{}]", info.player, info.governor);
      std::string idle_str = std::format("{}s", idle_seconds);

      std::vector<std::string> flags;
      if (r->governor[info.governor.value].toggle.gag) flags.push_back("GAG");
      if (r->governor[info.governor.value].toggle.invisible)
        flags.push_back("INVISIBLE");
      std::string flags_str;
      for (std::size_t i = 0; i < flags.size(); ++i) {
        if (i > 0) flags_str += " ";
        flags_str += flags[i];
      }

      table.add_row(
          {r->name, gov_name, player_gov, idle_str, star_name, flags_str});
    } else if (!is_god) {
      coward_count++;  // Non-God player sees someone invisible
    }
  }

  g.out << table << "\n";

  if (SHOW_COWARDS) {
    g.out << std::format("And {} coward{}.\n", coward_count,
                         (coward_count == 1) ? "" : "s");
  } else {
    g.out << "Finished.\n";
  }
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
