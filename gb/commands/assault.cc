// SPDX-License-Identifier: Apache-2.0

/// \file assault.cc
/// \brief Assault and attempt to capture a target ship.

module;

import std;
import gb.entities;
import gb.mechanics;
import gb.presentation.combat;
import gb.services;

module commands;

namespace GB::commands {

namespace {

struct ParsedAssaultArgs {
  PopulationType what{PopulationType::MIL};
  std::optional<population_t> boarders{std::nullopt};
};

std::optional<ParsedAssaultArgs> parse_assault_args(const command_t& argv,
                                                    GameObj& g) {
  if (argv.size() < 3) {
    g.out << "Assault what?\n";
    return std::nullopt;
  }
  if (argv.size() == 3) {
    return ParsedAssaultArgs{};
  }
  if (argv.size() == 4) {
    if (argv[3].starts_with("civ")) {
      return ParsedAssaultArgs{.what = PopulationType::CIV};
    }
    if (argv[3].starts_with("mil")) {
      return ParsedAssaultArgs{.what = PopulationType::MIL};
    }
    return ParsedAssaultArgs{
        .what = PopulationType::MIL,
        .boarders = parse_int<population_t>(argv[3]).value_or(population_t{0}),
    };
  }
  const population_t cnt =
      parse_int<population_t>(argv[3]).value_or(population_t{0});
  if (argv[4].starts_with("civ")) {
    return ParsedAssaultArgs{.what = PopulationType::CIV, .boarders = cnt};
  }
  if (argv[4].starts_with("mil")) {
    return ParsedAssaultArgs{.what = PopulationType::MIL, .boarders = cnt};
  }
  g.out << "Assault with what?\n";
  return std::nullopt;
}

void notify_boarding_outcome(GameObj& g, const BoardingOutcomeReport& b) {
  warn_player(g.session_registry, g.entity_manager, b.old_defender_owner,
              b.old_defender_gov,
              GB::presentation::render_boarding_defender_telegram(b));

  const std::string news = GB::presentation::render_boarding_news(b);
  if (b.captured || !b.defender_alive) {
    post(g.entity_manager, news, NewsType::COMBAT);
  }
  if (b.scope != ScopeLevel::LEVEL_UNIV) {
    notify_star(g.session_registry, g.entity_manager, g.player(), g.governor(),
                b.star_id, news);
  }
}

}  // namespace

bool assault(const command_t& argv, GameObj& g) {
  const auto parsed = parse_assault_args(argv, g);
  if (!parsed) {
    return false;
  }

  bool any_assaulted = false;
  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    const auto target_id = string_to_shipnum(argv[2]);
    if (!target_id) {
      g.out << "Invalid ship number.\n";
      return any_assaulted;
    }

    auto res = assault_single_ship(g.entity_manager, *ship_handle, *target_id,
                                   parsed->what, parsed->boarders, g.player(),
                                   g.governor(), g.god());
    if (!res) {
      g.out << GB::presentation::render_assault_error(res.error());
      if (res.error().abort_loop) {
        return any_assaulted;
      }
      continue;
    }

    if (res->defensive_fire) {
      notify_ship_combat_exchange(g, *res->defensive_fire,
                                  res->defensive_fire->shooter_owner,
                                  res->defensive_fire->shooter_governor);
    }
    if (res->boarding) {
      notify_boarding_outcome(g, *res->boarding);
    }
    g.present(*res);
    any_assaulted = true;
    if (res->abort_loop) {
      return any_assaulted;
    }
  }

  return any_assaulted;
}

const CommandDescriptor assault_cmd{
    .name = "assault",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "assault <ship> <target_ship> [<boarders>] [civilians|military]",
    .description = "Assault and attempt to capture a target ship",
    .handler = &assault,
};

}  // namespace GB::commands
