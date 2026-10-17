// SPDX-License-Identifier: Apache-2.0

/// \file tech_status.cc
/// \brief Technology investment and generation report for colonies.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace {

using GB::presentation::TechStatusRow;
using GB::presentation::TechStatusViewModel;

void tech_report_star(TechStatusViewModel& vm, GameObj& g, const Star& star,
                      starnum_t snum) {
  const player_t Playernum = g.player();
  const governor_t Governor = g.governor();

  if (!star.is_explored_by(Playernum) || !star.control(Playernum, Governor)) {
    return;
  }

  for (const auto& pl :
       PlanetList::readonly(g.entity_manager, snum, star.numplanets())) {
    if (!pl.info(Playernum).explored || !pl.info(Playernum).numsectsowned) {
      continue;
    }

    std::string location = std::format("{}/{}{}", star.get_name(),
                                       star.get_planet_name(pl.planet_order()),
                                       (pl.info(Playernum).autorep ? "*" : ""));

    auto gain =
        tech_prod(pl.info(Playernum).tech_invest, pl.info(Playernum).popn);
    auto max_gain =
        tech_prod(pl.info(Playernum).prod_res, pl.info(Playernum).popn);

    vm.rows.push_back(TechStatusRow{
        .location = std::move(location),
        .popn = pl.info(Playernum).popn,
        .tech_invest = pl.info(Playernum).tech_invest,
        .gain = gain,
        .max_gain = max_gain,
    });

    vm.total_invest += pl.info(Playernum).tech_invest;
    vm.total_gain += gain;
    vm.total_max_gain += max_gain;
  }
}
}  // namespace

namespace GB::commands {
bool tech_status(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  TechStatusViewModel vm{};

  if (argv.size() == 1) {
    for (const Star& star : StarList::readonly(g.entity_manager)) {
      tech_report_star(vm, g, star, star.star_id());
    }
  } else { /* Several arguments */
    for (std::size_t k = 1; k < argv.size(); k++) {
      auto where = Place::resolve(g.entity_manager, g.scope_context(), argv[k]);
      if (!where) {
        vm.errors.push_back(format_place_error(where.error()));
      }
      if (!where || where->level == ScopeLevel::LEVEL_UNIV ||
          where->level == ScopeLevel::LEVEL_SHIP) {
        vm.errors.push_back(std::format("Bad location `{}`.\n", argv[k]));
        continue;
      } /* ok, a proper location */
      starnum_t star = where->snum;
      try {
        const auto& star_ref = *g.entity_manager.peek_star(star);
        tech_report_star(vm, g, star_ref, star);
      } catch (const EntityNotFoundError&) {
        continue;
      }
    }
  }

  try {
    if (const auto* power_ptr = g.entity_manager.peek_power(Playernum)) {
      vm.total_popn = power_ptr->popn();
    }
  } catch (const EntityNotFoundError&) {
  }

  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
  return true;
}

const CommandDescriptor status_cmd{
    .name = "status",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "status [<star> ...]",
    .description = "Technology investment and generation report for colonies",
    .handler = &tech_status,
};

}  // namespace GB::commands