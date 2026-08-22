// SPDX-License-Identifier: Apache-2.0

/// \file whois.cc
/// \brief Identify players and race names.

module;

import gblib;
import std;

module commands;

namespace {
void display_whois(GameObj& g, player_t j) {
  const auto* race = g.entity_manager.peek_race(j);
  if (!race) {
    g.out << std::format("Race #{} not found.\n", j.value);
    return;
  }

  if (j == g.player()) {
    g.out << std::format("[{:2d}, {}] {} \"{}\"\n", j.value, g.governor().value,
                         race->name, race->governor[g.governor().value].name);
  } else {
    g.out << std::format("[{:2d}] {}\n", j.value, race->name);
  }
}
}  // namespace

namespace GB::commands {

bool whois(const command_t& argv, GameObj& g) {
  if (argv.size() <= 1) {
    display_whois(g, g.player());
    return true;
  }

  for (const auto& player_str :
       std::ranges::subrange(argv.begin() + 1, argv.end())) {
    player_t j = get_player(g.entity_manager, player_str);
    if (j == player_t{0}) {
      g.out << std::format("Identify: Invalid player {}. Try again.\n",
                           player_str);
      continue;
    }
    display_whois(g, j);
  }
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