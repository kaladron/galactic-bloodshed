// SPDX-License-Identifier: Apache-2.0

/// \file explore.cc
/// \brief Global census and exploration survey of known stars and worlds.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace GB::commands {
bool explore(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  std::optional<starnum_t> target_star{std::nullopt};

  if (argv.size() == 2) {
    auto where = Place::resolve(g.entity_manager, g.scope_context(), argv[1]);
    if (!where) {
      g.out << format_place_error(where.error());
      g.out << "explore: bad scope.\n";
      return false;
    }
    if (where->level == ScopeLevel::LEVEL_SHIP ||
        where->level == ScopeLevel::LEVEL_UNIV) {
      g.out << std::format("Bad scope '{}'\n", argv[1]);
      return false;
    }
    target_star = where->snum;
  }

  const auto& sdata = *g.entity_manager.peek_universe();
  GB::presentation::ExploreViewModel vm{
      .global_ap = sdata.get_AP(Playernum),
  };

  for (const Star& star_ref : StarList::readonly(g.entity_manager)) {
    if (target_star && (*target_star != star_ref.star_id())) {
      continue;
    }
    if (!star_ref.is_explored_by(Playernum)) {
      continue;
    }

    GB::presentation::ExploreStarEntry star_entry{
        .star_name = star_ref.get_name(),
        .stability = (g.race->tech >= TECH_SEE_STABILITY)
                         ? std::optional<int>{star_ref.stability()}
                         : std::nullopt,
        .ap = star_ref.AP(Playernum),
    };

    for (const Planet& pl :
         PlanetList::readonly(g.entity_manager, star_ref.star_id(), star_ref)) {
      const planetnum_t pnum = pl.planet_order();
      const auto& pinfo = pl.info(Playernum);
      GB::presentation::ExplorePlanetRow row{
          .planet_order = pnum,
          .planet_name = star_ref.get_planet_name(pnum),
          .explored = pinfo.explored,
      };
      if (row.explored) {
        row.autorep = (pinfo.autorep != 0);
        row.inhabited = (pinfo.numsectsowned > 0);
        row.enslaved = pl.is_enslaved();
        for (const auto& [other_player, info] : pl.info_map()) {
          if (other_player != Playernum && info.numsectsowned > 0) {
            row.alien_owners.push_back(other_player);
          }
        }
        row.toxic = (pl.toxic() > 70);
        row.type_name = pl.type_name();
        row.compatibility = pl.compatibility(*g.race);
      }
      star_entry.planets.push_back(std::move(row));
    }

    vm.stars.push_back(std::move(star_entry));
  }

  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
  return true;
}

const CommandDescriptor explore_cmd{
    .name = "explore",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "explore [<where>]",
    .description =
        "Global census and exploration survey of known stars and worlds",
    .handler = &explore,
};

}  // namespace GB::commands
