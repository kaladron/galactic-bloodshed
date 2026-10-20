// SPDX-License-Identifier: Apache-2.0

/// \file bombard.cc
/// \brief Ship vs planet bombardment command.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation.combat;
import gb.services;
import scnlib;
import std;

module commands;

namespace {

void notify_retaliation_shot(GameObj& g, starnum_t star_id,
                             player_t recipient_owner,
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

void notify_bombard_result(GameObj& g, const BombardShipResult& res) {
  if (res.bomber_overload) {
    GB::commands::notify_reactor_overload(g.entity_manager,
                                          *res.bomber_overload);
  }
  if (res.bombard) {
    const std::string short_msg =
        GB::presentation::render_bombard_short(*res.bombard);
    const std::string long_msg =
        GB::presentation::render_bombard_long(*res.bombard);
    post(g.entity_manager, short_msg, NewsType::COMBAT);
    notify_star(g.session_registry, g.entity_manager, g.player(), g.governor(),
                res.star_id, short_msg);
    const auto* star = g.entity_manager.peek_star(res.star_id);
    for (player_t i : res.bombard->nuked_players) {
      warn_player(g.session_registry, g.entity_manager, i, star->governor(i),
                  long_msg);
    }
  }
  for (const auto& salvo : res.planet_retaliation) {
    notify_retaliation_shot(g, res.star_id, salvo.defender_player,
                            salvo.defender_governor, salvo.shot);
  }
  for (const auto& prot : res.protector_retaliation) {
    if (prot.overload) {
      GB::commands::notify_reactor_overload(g.entity_manager, *prot.overload);
    }
    if (prot.shot) {
      notify_retaliation_shot(g, res.star_id, prot.escort_owner,
                              prot.escort_governor, *prot.shot);
    }
  }
}

}  // namespace

namespace GB::commands {

/*! Ship vs planet */
bool bombard(const command_t& argv, GameObj& g) {
  const std::optional<std::string_view> coords_arg =
      argv.size() > 2 ? std::optional<std::string_view>{argv[2]} : std::nullopt;
  std::optional<weapon_power_t> requested_strength;
  if (argv.size() > 3) {
    auto parsed = scn::scan<weapon_power_t>(argv[3], "{}");
    requested_strength = parsed ? parsed->value() : -1;
  }

  bool any_fired = false;
  for (auto ship_handle : ScopedCommandableShips(g, argv[1])) {
    auto res = bombard_single_ship(g.entity_manager, *ship_handle, coords_arg,
                                   requested_strength, g.player(), g.god());
    if (!res) {
      g.out << GB::presentation::format_bombard_error(res.error());
      continue;
    }
    notify_bombard_result(g, *res);
    g.present(*res);
    any_fired = true;
  }

  return any_fired;
}

const CommandDescriptor bombard_cmd{
    .name = "bombard",
    .roles =
        {
            .no_guests = true,
        },
    .scopes = AllowedScopes::planet_or_ship(),
    .ap = APCost::dynamic(),
    .min_args = 2,
    .syntax = "bombard <ship> [<x,y> [<strength>]]",
    .description = "Bombard planetary sectors from orbiting or AFV ships",
    .handler = &bombard,
};

}  // namespace GB::commands
