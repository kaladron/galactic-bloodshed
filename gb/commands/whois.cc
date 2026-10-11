// SPDX-License-Identifier: Apache-2.0

/// \file whois.cc
/// \brief Identify players and race names.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace {
GB::presentation::WhoisEntry build_whois_entry(GameObj& g, player_t target) {
  try {
    return g.entity_manager.with_race(target, [&](const Race& race) {
      if (target == g.player()) {
        return GB::presentation::WhoisEntry{
            .player = target,
            .governor = g.governor(),
            .race_name = race.name,
            .governor_name = race.governor(g.governor()).name,
        };
      }
      return GB::presentation::WhoisEntry{
          .player = target,
          .race_name = race.name,
      };
    });
  } catch (const EntityNotFoundError&) {
    return GB::presentation::WhoisEntry{
        .player = target,
        .error = std::format("Race #{} not found.", target),
    };
  }
}
}  // namespace

namespace GB::commands {

bool whois(const command_t& argv, GameObj& g) {
  GB::presentation::WhoisViewModel vm{};
  if (argv.size() <= 1) {
    vm.entries.push_back(build_whois_entry(g, g.player()));
    g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
    return true;
  }

  for (const auto& player_str :
       std::ranges::subrange(argv.begin() + 1, argv.end())) {
    const player_t target = get_player(g.entity_manager, player_str);
    if (target == player_t{0}) {
      vm.entries.push_back(GB::presentation::WhoisEntry{
          .error = std::format("Identify: Invalid player {}. Try again.",
                               player_str),
      });
      continue;
    }
    vm.entries.push_back(build_whois_entry(g, target));
  }
  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
  return true;
}

namespace {
constexpr std::string_view whois_aliases[] = {"identify"};
}

const CommandDescriptor whois_cmd{
    .name = "whois",
    .aliases = whois_aliases,
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "whois [<player> ...]",
    .description = "Identify player race names and numbers",
    .handler = &whois,
};

}  // namespace GB::commands