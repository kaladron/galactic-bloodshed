// SPDX-License-Identifier: Apache-2.0

/// \file map.cc
/// \brief Cartesian coordinate map of a planet's surface or orbit display.

module;

import gb.entities;
import gb.mechanics;
import gb.presentation.planet_views;
import gb.services;
import std;
#undef stdout

module commands;

namespace GB::commands {
bool map(const command_t& argv, GameObj& g) {
  auto where = Place::resolve(g.entity_manager, g.scope_context(),
                              argv.size() > 1 ? argv[1] : std::string_view{});
  if (!where) {
    g.out << format_place_error(where.error());
    return false;
  }

  switch (where->level) {
    case ScopeLevel::LEVEL_SHIP:
      g.out << "Bad scope.\n";
      return false;
    case ScopeLevel::LEVEL_PLAN: {
      const auto& p = *g.entity_manager.peek_planet(where->snum, where->pnum);
      g.present(GB::presentation::PlanetMapOutputViewModel{
          .ascii_map =
              build_planet_map(g.entity_manager, where->snum, where->pnum, p,
                               g.player(), g.governor(), *g.race),
          .planet = GB::presentation::project_planet_view(
              g.entity_manager, g.scope_context(), where->snum, where->pnum),
          .sectors = GB::presentation::project_sector_grid_view(
              g.entity_manager, g.scope_context(), where->snum, where->pnum),
      });
      return true;
    }
    default:
      return orbit(argv, g); /* make orbit map instead */
  }
}

const CommandDescriptor map_cmd{
    .name = "map",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "map [<path>]",
    .description =
        "Cartesian coordinate map of a planet's surface or orbit display",
    .handler = &map,
};

}  // namespace GB::commands
