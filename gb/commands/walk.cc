// SPDX-License-Identifier: Apache-2.0

/// \file walk.cc
/// \brief Walk command implementation.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation.combat;
import gb.services;
import std;

module commands;

namespace {

void notify_walk_result(GameObj& g, const WalkResult& res) {
  for (const auto& duel : res.afv_duel_shots) {
    const auto short_buf = GB::presentation::render_ship_shot_short(duel.shot);
    const auto long_buf = GB::presentation::render_ship_shot_long(duel.shot);
    warn_player(g.session_registry, g.entity_manager, duel.defender_owner,
                duel.defender_governor, long_buf);
    if (!duel.shot.target_alive) {
      post(g.entity_manager, short_buf, NewsType::COMBAT);
    }
    notify_star(g.session_registry, g.entity_manager, g.player(), g.governor(),
                res.star_id, short_buf);
  }

  for (const auto& round : res.garrison_rounds) {
    const auto short_buf =
        GB::presentation::render_mech_attack_people_short(round.mech_attack);
    const auto long_buf =
        GB::presentation::render_mech_attack_people_long(round.mech_attack);
    warn_player(g.session_registry, g.entity_manager, round.defender_player,
                round.defender_governor, long_buf);
    notify_star(g.session_registry, g.entity_manager, g.player(), g.governor(),
                res.star_id, short_buf);
    post(g.entity_manager, short_buf, NewsType::COMBAT);

    const auto short_buf2 = GB::presentation::render_people_attack_mech_short(
        round.people_counterattack);
    const auto long_buf2 = GB::presentation::render_people_attack_mech_long(
        round.people_counterattack);
    warn_player(g.session_registry, g.entity_manager, round.defender_player,
                round.defender_governor, long_buf2);
    notify_star(g.session_registry, g.entity_manager, g.player(), g.governor(),
                res.star_id, short_buf2);
    if (!round.people_counterattack.mech_alive) {
      post(g.entity_manager, short_buf2, NewsType::COMBAT);
    }
  }

  if (res.advanced && !res.move_observers.empty()) {
    const auto moving = GB::presentation::render_walk_observer_warning(res);
    for (const auto& obs : res.move_observers) {
      warn_player(g.session_registry, g.entity_manager, obs.player,
                  obs.governor, moving);
    }
  }
}

}  // namespace

namespace GB::commands {

bool walk(const command_t& argv, GameObj& g) {
  const auto shipno = string_to_shipnum(argv[1]);
  if (!shipno || *shipno <= 0) {
    g.out << "Bad ship number.\n";
    return false;
  }

  const char direction = argv[2].empty() ? '\0' : argv[2][0];
  auto res = execute_afv_walk(g.entity_manager, *shipno, direction, g.player(),
                              g.governor(), g.god());
  if (!res) {
    g.out << GB::presentation::format_walk_error(res.error());
    return false;
  }

  notify_walk_result(g, *res);
  g.present(*res);
  return true;
}

const CommandDescriptor walk_cmd{
    .name = "walk",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "walk <ship> <direction>",
    .description = "Move an AFV from one sector to another",
    .handler = &walk,
};

}  // namespace GB::commands
