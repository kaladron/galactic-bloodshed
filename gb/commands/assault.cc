// SPDX-License-Identifier: Apache-2.0

/// \file assault.cc
/// \brief Assault and attempt to capture a target ship.

module;

import std;
import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import scnlib;

module commands;

namespace GB::commands {

namespace {

std::optional<PopulationType>
parse_assault_population_type(const command_t& argv, GameObj& g) {
  if (argv.size() < 3) {
    g.out << "Assault what?\n";
    return std::nullopt;
  }
  if (argv.size() < 5) {
    return PopulationType::MIL;
  }
  if (argv[4].starts_with("civ")) {
    return PopulationType::CIV;
  }
  if (argv[4].starts_with("mil")) {
    return PopulationType::MIL;
  }
  g.out << "Assault with what?\n";
  return std::nullopt;
}

std::optional<population_t> parse_requested_boarders(const command_t& argv) {
  if (argv.size() < 4) {
    return std::nullopt;
  }
  if (auto scan_res = scn::scan<population_t>(argv[3], "{}")) {
    return scan_res->value();
  }
  return std::nullopt;
}

void notify_combat_shot(GameObj& g, starnum_t star_id, player_t recipient_owner,
                        governor_t recipient_governor,
                        const ShipShotResult& shot) {
  const std::string short_buf = GB::presentation::render_ship_shot_short(shot);
  const std::string long_buf = GB::presentation::render_ship_shot_long(shot);
  if (!shot.target_alive) {
    post(g.entity_manager, short_buf, NewsType::COMBAT);
  }
  notify_star(g.session_registry, g.entity_manager, g.player(), g.governor(),
              star_id, short_buf);
  warn_player(g.session_registry, g.entity_manager, recipient_owner,
              recipient_governor, long_buf);
}

void notify_defensive_fire(GameObj& g, const ShipCombatExchange& df) {
  if (df.primary_overload) {
    notify_reactor_overload(g.entity_manager, *df.primary_overload);
  }
  if (df.primary_shot) {
    notify_combat_shot(g, df.star_id, df.shooter_owner, df.shooter_governor,
                       *df.primary_shot);
  }

  if (df.retaliation_overload) {
    notify_reactor_overload(g.entity_manager, *df.retaliation_overload);
  }
  if (df.retaliation_shot) {
    notify_combat_shot(g, df.star_id, df.shooter_owner, df.shooter_governor,
                       *df.retaliation_shot);
  }

  for (const auto& escort : df.escort_shots) {
    if (escort.overload) {
      notify_reactor_overload(g.entity_manager, *escort.overload);
    }
    if (escort.shot) {
      notify_combat_shot(g, df.star_id, escort.escort_owner,
                         escort.escort_governor, *escort.shot);
    }
  }
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
  const auto what_opt = parse_assault_population_type(argv, g);
  if (!what_opt) {
    return false;
  }

  const auto requested_boarders = parse_requested_boarders(argv);

  bool any_assaulted = false;
  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    const auto target_id = string_to_shipnum(argv[2]);
    if (!target_id) {
      g.out << "Invalid ship number.\n";
      return any_assaulted;
    }

    auto res = assault_single_ship(g.entity_manager, *ship_handle, *target_id,
                                   *what_opt, requested_boarders, g.player(),
                                   g.governor(), g.god());
    if (!res) {
      g.out << GB::presentation::render_assault_error(res.error());
      if (res.error().abort_loop) {
        return any_assaulted;
      }
      continue;
    }

    if (res->defensive_fire) {
      notify_defensive_fire(g, *res->defensive_fire);
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
