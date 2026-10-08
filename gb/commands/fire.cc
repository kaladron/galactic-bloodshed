// SPDX-License-Identifier: Apache-2.0

/// \file fire.cc
/// \brief Fire conventional or laser weapons at target ship.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {

namespace {

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

}  // namespace

void notify_reactor_overload(EntityManager& em,
                             const ReactorOverloadEvent& event) {
  const std::string message =
      GB::presentation::render_reactor_overload_event(event);
  push_telegram(em, event.owner, event.governor, message);
  if (event.outcome == ReactorOverloadOutcome::ShipExploded) {
    post(em, message, NewsType::COMBAT);
    if (event.scope != ScopeLevel::LEVEL_UNIV) {
      telegram_star(em, event.star_id, event.owner, event.governor, message);
    }
  }
}

void notify_ship_combat_exchange(GameObj& g, const ShipCombatExchange& df,
                                 player_t other_owner,
                                 governor_t other_governor) {
  if (df.primary_overload) {
    notify_reactor_overload(g.entity_manager, *df.primary_overload);
  }
  if (df.primary_shot) {
    notify_combat_shot(g, df.star_id, other_owner, other_governor,
                       *df.primary_shot);
  }

  if (df.retaliation_overload) {
    notify_reactor_overload(g.entity_manager, *df.retaliation_overload);
  }
  if (df.retaliation_shot) {
    notify_combat_shot(g, df.star_id, other_owner, other_governor,
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

bool fire(const command_t& argv, GameObj& g) {
  if (argv.size() < 3) {
    g.out << "Syntax: 'fire <ship> <target> [<strength>]'.\n";
    return false;
  }

  const auto toshiptmp = string_to_shipnum(argv[2]);
  if (!toshiptmp || *toshiptmp <= 0) {
    g.out << "Bad ship number.\n";
    return false;
  }
  const shipnum_t toship = *toshiptmp;

  std::optional<weapon_power_t> requested_strength;
  if (argv.size() >= 4) {
    auto parsed = scn::scan<weapon_power_t>(argv[3], "{}");
    if (!parsed) {
      g.out << "No attack.\n";
      return false;
    }
    requested_strength = parsed->value();
  }

  bool any_fired = false;
  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    auto res = fire_single_ship(g.entity_manager, *ship_handle, toship,
                                requested_strength, g.player(), g.god());
    if (!res) {
      g.out << GB::presentation::render_fire_error(res.error());
      if (res.error().abort_loop) {
        return any_fired;
      }
      continue;
    }

    notify_ship_combat_exchange(g, res->exchange, res->exchange.target_owner,
                                res->exchange.target_governor);
    g.present(*res);
    any_fired = true;
  }

  return any_fired;
}

const CommandDescriptor fire_cmd{
    .name = "fire",
    .roles =
        {
            .no_guests = true,
        },
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "fire <ship> <target> [<strength>]",
    .description = "Fire conventional or laser weapons at target ship",
    .handler = &fire,
};

}  // namespace GB::commands
