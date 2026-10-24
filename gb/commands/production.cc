// SPDX-License-Identifier: Apache-2.0

/// \file production.cc
/// \brief Display planet and sector production estimates.

module;

import gb.entities;
import gb.presentation.planet_views;
import gb.services;
import std;

module commands;

namespace {

void append_planet_production(
    const Star& star_ref, const Planet& pl, player_t player,
    governor_t governor, std::vector<GB::presentation::ProductionRow>& rows) {
  const auto& info = pl.info(player);
  if (!info.explored || !info.numsectsowned ||
      !star_ref.control(player, governor)) {
    return;
  }
  rows.push_back({
      .snum = star_ref.star_id(),
      .pnum = pl.planet_order(),
      .star_name = star_ref.get_name(),
      .planet_name = star_ref.get_planet_name(pl.planet_order()),
      .type_symbol = pl.type_symbol(),
      .autoreport = info.autorep != 0,
      .governor = star_ref.governor(player),
      .prod_tech = info.prod_tech,
      .total_resources = pl.total_resources(),
      .prod_crystals = info.prod_crystals,
      .prod_res = info.prod_res,
      .prod_dest = info.prod_dest,
      .prod_fuel = info.prod_fuel,
      .prod_money = info.prod_money,
      .tox_thresh = info.tox_thresh.value_or(0),
      .est_production = info.est_production,
  });
}

void append_star_production(
    GameObj& g, starnum_t star,
    std::vector<GB::presentation::ProductionRow>& rows) {
  const auto& star_ref = *g.entity_manager.peek_star(star);
  if (!star_ref.is_explored_by(g.player())) return;

  for (const auto& pl :
       PlanetList::readonly(g.entity_manager, star, star_ref.numplanets())) {
    append_planet_production(star_ref, pl, g.player(), g.governor(), rows);
  }
}

}  // namespace

namespace GB::commands {
bool production(const command_t& argv, GameObj& g) {
  GB::presentation::ProductionReportViewModel vm{};

  if (argv.size() < 2) {
    for (const Star& star : StarList::readonly(g.entity_manager)) {
      append_star_production(g, star.star_id(), vm.rows);
    }
  } else {
    for (std::size_t i = 1; i < argv.size(); ++i) {
      auto where = Place::resolve(g.entity_manager, g.scope_context(), argv[i]);
      if (!where || where->level == ScopeLevel::LEVEL_UNIV ||
          where->level == ScopeLevel::LEVEL_SHIP) {
        vm.bad_locations.push_back(argv[i]);
        continue;
      }
      if (where->level == ScopeLevel::LEVEL_PLAN) {
        const auto& star_ref = *g.entity_manager.peek_star(where->snum);
        if (star_ref.is_explored_by(g.player())) {
          const auto& pl =
              *g.entity_manager.peek_planet(where->snum, where->pnum);
          append_planet_production(star_ref, pl, g.player(), g.governor(),
                                   vm.rows);
        }
      } else {
        append_star_production(g, where->snum, vm.rows);
      }
    }
  }

  g.present(vm);
  return true;
}

const CommandDescriptor production_cmd{
    .name = "production",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "production [<location> ...]",
    .description = "Display planet and sector production estimates",
    .handler = &production,
};

}  // namespace GB::commands
