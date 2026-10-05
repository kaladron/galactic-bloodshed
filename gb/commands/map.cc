// SPDX-License-Identifier: Apache-2.0

/// \file map.cc
/// \brief Cartesian coordinate map of a planet's surface or orbit display.

module;

import gb.entities;
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
      const auto* p = g.entity_manager.peek_planet(where->snum, where->pnum);
      if (!p) {
        g.out << "Planet not found.\n";
        return false;
      }
      show_map(g, where->snum, where->pnum, *p);
      const auto& star = *g.entity_manager.peek_star(where->snum);
      if (star.stability() > 50)
        g.out << "WARNING! This planet's primary is unstable.\n";
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
