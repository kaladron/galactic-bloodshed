// SPDX-License-Identifier: Apache-2.0

/// \file colonies.cc
/// \brief Colonization report command.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace {

using GB::presentation::ColoniesViewModel;
using GB::presentation::ColonyRow;

void colonies_at_star(ColoniesViewModel& vm, GameObj& g, const Race& race,
                      const starnum_t star) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  const auto& star_ref = *g.entity_manager.peek_star(star);
  if (!star_ref.is_explored_by(Playernum)) return;

  for (const auto& pl :
       PlanetList::readonly(g.entity_manager, star, star_ref.numplanets())) {
    if (!pl.info(Playernum).explored || !pl.info(Playernum).numsectsowned ||
        !star_ref.control(Playernum, Governor)) {
      continue;
    }

    ColonyRow row{
        .type_symbol = pl.type_symbol(),
        .star_name = star_ref.get_name(),
        .planet_name = star_ref.get_planet_name(pl.planet_order()),
        .autorep = pl.info(Playernum).autorep != 0,
        .governor = star_ref.governor(Playernum),
        .sectors_owned = pl.info(Playernum).numsectsowned,
        .tech_invest = pl.info(Playernum).tech_invest,
        .popn = pl.info(Playernum).popn,
        .crystals = pl.info(Playernum).crystals,
        .resource = pl.info(Playernum).resource,
        .destruct = pl.info(Playernum).destruct,
        .fuel = pl.info(Playernum).fuel,
        .tax = pl.info(Playernum).tax,
        .newtax = pl.info(Playernum).newtax,
        .compatibility = pl.compatibility(race),
        .toxicity = pl.toxic(),
        .comread = pl.info(Playernum).comread,
        .mob_set = pl.info(Playernum).mob_set,
    };
    for (const auto& [j, info] : pl.info_map()) {
      if ((j != Playernum) && (info.numsectsowned > 0)) {
        row.alien_owners.push_back(j);
      }
    }
    vm.rows.push_back(std::move(row));
  }
}
}  // namespace

namespace GB::commands {

bool colonies(const command_t& argv, GameObj& g) {
  ColoniesViewModel vm{};

  if (argv.size() < 2) {
    for (const Star& star : StarList::readonly(g.entity_manager)) {
      colonies_at_star(vm, g, *g.race, star.star_id());
    }
  } else {
    for (std::size_t i = 1; i < argv.size(); i++) {
      auto where = Place::resolve(g.entity_manager, g.scope_context(), argv[i]);
      if (!where) {
        vm.errors.push_back(format_place_error(where.error()));
      }
      if (!where || (where->level == ScopeLevel::LEVEL_UNIV) ||
          (where->level == ScopeLevel::LEVEL_SHIP)) {
        vm.errors.push_back(std::format("Bad location `{}'.\n", argv[i]));
        continue;
      }
      colonies_at_star(vm, g, *g.race, where->snum);
    }
  }
  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
  return true;
}

const CommandDescriptor colonies_cmd{
    .name = "colonies",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "colonies [<star> ...]",
    .description =
        "Display colonization report across all or specified star systems",
    .handler = &colonies,
};

}  // namespace GB::commands
