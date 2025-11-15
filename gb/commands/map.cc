// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void map(const command_t &argv, GameObj &g) {
  std::unique_ptr<Place> where;

  if (argv.size() > 1) {
    where = std::make_unique<Place>(g, argv[1]);
  } else {
    where = std::make_unique<Place>(g, "");
  }

  if (where->err) return;

  switch (where->level) {
    case ScopeLevel::LEVEL_SHIP:
      g.out << "Bad scope.\n";
      return;
    case ScopeLevel::LEVEL_PLAN: {
      const auto* p = g.entity_manager.peek_planet(where->snum, where->pnum);
      if (!p) {
        g.out << "Planet not found.\n";
        return;
      }
      show_map(g, where->snum, where->pnum, *p);
      const auto* star = g.entity_manager.peek_star(where->snum);
      if (star && star->stability() > 50)
        g.out << "WARNING! This planet's primary is unstable.\n";
    } break;
    default:
      orbit(argv, g); /* make orbit map instead */
  }
}
}  // namespace GB::commands
