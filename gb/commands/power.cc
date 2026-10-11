// SPDX-License-Identifier: Apache-2.0
/// \file power.cc
/// \brief Display power report command.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace {

using GB::presentation::PowerRow;
using GB::presentation::PowerViewModel;

void add_power_row(PowerViewModel& vm, EntityManager& em, const Race& race,
                   const Race& r, player_t i, int rank) {
  std::string alliance_them = race.is_allied_with(i)   ? "+"
                              : race.is_at_war_with(i) ? "-"
                                                       : " ";
  std::string alliance_us = r.is_allied_with(race.Playernum)   ? "+"
                            : r.is_at_war_with(race.Playernum) ? "-"
                                                               : " ";

  const power* power_ptr = nullptr;
  try {
    power_ptr = em.peek_power(i);
  } catch (const EntityNotFoundError&) {
    return;
  }

  std::string know_col;
  if (race.God) {
    const auto* universe = em.peek_universe();
    know_col = std::format("{}", universe->vn_hits(i));
  } else {
    know_col = std::format("{}%", race.translation_for(i));
  }

  vm.rows.push_back(PowerRow{
      .rank = rank,
      .player = i,
      .alliance = alliance_them + alliance_us,
      .name = std::string(r.name),
      .vp = race.estimate(r.victory_score, i),
      .mil = race.estimate(power_ptr->troops, i),
      .civ = race.estimate(power_ptr->popn, i),
      .cash = race.estimate(power_ptr->money, i),
      .ships = race.estimate(power_ptr->ships_owned, i),
      .planets = race.estimate(power_ptr->planets_owned, i),
      .resource = race.estimate(power_ptr->resource, i),
      .fuel = race.estimate(power_ptr->fuel, i),
      .destruct = race.estimate(power_ptr->destruct, i),
      .morale = race.estimate(r.morale, i),
      .know_or_vns = std::move(know_col),
  });
}
}  // namespace

namespace GB::commands {
bool power(const command_t& argv, GameObj& g) {
  std::optional<player_t> target_player;

  if (argv.size() >= 2) {
    player_t p = get_player(g.entity_manager, argv[1]);
    if (p == 0) {
      g.out << "No such player,\n";
      return false;
    }
    target_player = p;
  }

  const auto* race = g.race;
  PowerViewModel vm{
      .show_rank_header = argv.size() < 2,
      .is_god = race->God,
  };

  if (argv.size() < 2) {
    auto vicvec = create_victory_list(g.entity_manager);
    int rank = 0;
    for (const auto& vic : vicvec) {
      rank++;
      player_t p = vic.racenum;
      const auto* r = g.entity_manager.peek_race(p);
      if (!r) continue;
      if (!r->dissolved && race->translation_for(p) >= 10) {
        add_power_row(vm, g.entity_manager, *race, *r, p, rank);
      }
    }
  } else {
    const auto* r = g.entity_manager.peek_race(target_player.value());
    if (!r) {
      g.out << "Race not found.\n";
      return false;
    }
    add_power_row(vm, g.entity_manager, *race, *r, target_player.value(), 0);
  }

  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
  return true;
}

const CommandDescriptor power_cmd{
    .name = "power",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "power [<player>]",
    .description = "Galactic influence, stockpiles, and military power report",
    .handler = &power,
};

}  // namespace GB::commands
