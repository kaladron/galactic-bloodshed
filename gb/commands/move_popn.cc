// SPDX-License-Identifier: Apache-2.0

/// \file move_popn.cc
/// \brief Move civilian population or deploy military troops across sectors.

module;

import gb.entities;
import gb.presentation.combat;
import gb.services;
import scnlib;
import std;

module commands;

namespace GB::commands {

bool move_popn(const command_t& argv, GameObj& g) {
  const PopulationType what =
      (argv[0] == "move") ? PopulationType::CIV : PopulationType::MIL;
  std::optional<population_t> requested_count;
  if (argv.size() >= 4) {
    auto count_res = scn::scan<population_t>(argv[3], "{}");
    requested_count = count_res ? count_res->value() : 0;
  }

  const auto res = execute_planet_sector_movement(
      g.entity_manager, g.snum(), g.pnum(), g.player(), g.god(), argv[1],
      argv[2], requested_count, what);
  if (!res) {
    g.out << GB::presentation::format_move_popn_error(res.error());
    return false;
  }

  for (const auto& step : res->steps) {
    for (const auto& round : step.mech_defense.rounds) {
      const auto attack_long =
          GB::presentation::render_mech_attack_people_long(round.mech_attack);
      push_telegram(g.entity_manager, g.player(), g.governor(), attack_long);
      push_telegram(g.entity_manager, round.defender_player,
                    round.defender_governor, attack_long);
      if (round.people_counterattack) {
        const auto counter_long =
            GB::presentation::render_people_attack_mech_long(
                *round.people_counterattack);
        push_telegram(g.entity_manager, g.player(), g.governor(), counter_long);
        push_telegram(g.entity_manager, round.defender_player,
                      round.defender_governor, counter_long);
      }
    }
    if (step.assault) {
      if (auto absorb_warn =
              GB::presentation::render_sector_ground_assault_absorb_warning(
                  *step.assault)) {
        warn_player(g.session_registry, g.entity_manager,
                    step.assault->defender_player,
                    step.assault->defender_governor, *absorb_warn);
      }
      warn_player(g.session_registry, g.entity_manager,
                  step.assault->defender_player,
                  step.assault->defender_governor,
                  GB::presentation::render_sector_ground_assault_telegram(
                      *step.assault));
    }
  }

  if (res->any_moved) {
    g.present(*res);
  } else {
    g.out << GB::presentation::render_move_popn_result(*res);
  }
  return res->any_moved;
}

bool deploy(const command_t& argv, GameObj& g) {
  return move_popn(argv, g);
}

const CommandDescriptor move_cmd{
    .name = "move",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "move <from_sector> <path> [<amount>]",
    .description = "Move civilian population across planetary sectors",
    .handler = &move_popn,
};

const CommandDescriptor deploy_cmd{
    .name = "deploy",
    .roles =
        {
            .star_control = true,
        },
    .scopes = AllowedScopes::planet_only(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "deploy <from_sector> <path> [<amount>]",
    .description = "Deploy military troops across planetary sectors",
    .handler = &deploy,
};

}  // namespace GB::commands
