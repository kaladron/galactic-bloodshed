// SPDX-License-Identifier: Apache-2.0

module;

import gb.entities;
import gb.mechanics;
import gb.presentation.combat;
import gb.services;
import scnlib;
import std;

module commands;

namespace {

void notify_defend_result(GameObj& g, const DefendPlanetResult& res) {
  const std::string p_short =
      GB::presentation::render_ship_shot_short(res.planet_shot);
  const std::string p_long =
      GB::presentation::render_ship_shot_long(res.planet_shot);
  if (!res.planet_shot.target_alive) {
    post(g.entity_manager, p_short, NewsType::COMBAT);
  }
  notify_star(g.session_registry, g.entity_manager, g.player(), g.governor(),
              res.star_id, p_short);
  warn_player(g.session_registry, g.entity_manager, res.target_owner,
              res.target_governor, p_long);

  for (const auto& ret : res.ship_retaliation) {
    if (ret.overload) {
      GB::commands::notify_reactor_overload(g.entity_manager, *ret.overload);
    }
    if (ret.bombard) {
      const std::string short_msg =
          GB::presentation::render_bombard_short(*ret.bombard);
      const std::string long_msg =
          GB::presentation::render_bombard_long(*ret.bombard);
      post(g.entity_manager, short_msg, NewsType::COMBAT);
      notify_star(g.session_registry, g.entity_manager, g.player(),
                  g.governor(), res.star_id, short_msg);
      warn_player(g.session_registry, g.entity_manager, ret.ship_owner,
                  ret.ship_governor, long_msg);
    }
  }
}

}  // namespace

namespace GB::commands {
/*! Planet vs ship */
bool defend(const command_t& argv, GameObj& g) {
  const auto toshiptmp = string_to_shipnum(argv[1]);
  if (!toshiptmp || *toshiptmp <= 0) {
    g.out << "Bad ship number.\n";
    return false;
  }

  std::optional<weapon_power_t> requested_strength;
  if (argv.size() >= 4) {
    auto parsed = scn::scan<weapon_power_t>(argv[3], "{}");
    requested_strength =
        parsed && parsed->value() > 0 ? parsed->value() : weapon_power_t{0};
  }

  auto res = defend_planet_against_ship(g.entity_manager, g.snum(), g.pnum(),
                                        g.player(), *toshiptmp, argv[2],
                                        requested_strength);
  if (!res) {
    g.out << GB::presentation::format_defend_error(res.error());
    return false;
  }

  notify_defend_result(g, *res);
  g.present(*res);
  return true;
}

const CommandDescriptor defend_cmd{
    .name = "defend",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::fixed_star(1),
    .min_args = 3,
    .syntax = "defend <ship> <sector> [<strength>]",
    .description =
        "Defend planet against orbiting ships using planetary defense guns",
    .handler = &defend,
};

}  // namespace GB::commands
