// SPDX-License-Identifier: Apache-2.0

/// \file distance.cc
/// \brief Calculate distance between stars, planets, or coordinates.

module;

import std;
import gblib;

module commands;

namespace GB::commands {
bool distance(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player();
  double x0;
  double y0;
  double x1;
  double y1;
  double dist;

  if (argv.size() < 3) {
    g.out << "Syntax: 'distance <from> <to>'.\n";
    return false;
  }

  Place from{g, argv[1], true};
  if (from.err) {
    g.out << std::format("Bad scope '{}'\n", argv[1]);
    return false;
  }
  Place to{g, argv[2], true};
  if (to.err) {
    g.out << std::format("Bad scope '{}'\n", argv[2]);
    return false;
  }

  x0 = 0.0;
  y0 = 0.0;
  x1 = 0.0;
  y1 = 0.0;
  /* get position in absolute units */
  if (from.level == ScopeLevel::LEVEL_SHIP) {
    const Ship* ship;
    try {
      ship = g.entity_manager.peek_ship(from.shipno);
    } catch (const EntityNotFoundError&) {
      g.out << "Ship not found.\n";
      return false;
    }
    if (ship->owner() != Playernum) {
      g.out << "Nice try.\n";
      return false;
    }
    x0 = ship->xpos();
    y0 = ship->ypos();
  } else if (from.level == ScopeLevel::LEVEL_PLAN) {
    const auto* p = g.entity_manager.peek_planet(from.snum, from.pnum);
    if (!p) {
      g.out << "Planet not found.\n";
      return false;
    }
    const auto* star = g.entity_manager.peek_star(from.snum);
    if (!star) {
      g.out << "Star not found.\n";
      return false;
    }
    x0 = p->xpos() + star->xpos();
    y0 = p->ypos() + star->ypos();
  } else if (from.level == ScopeLevel::LEVEL_STAR) {
    const auto* star = g.entity_manager.peek_star(from.snum);
    if (!star) {
      g.out << "Star not found.\n";
      return false;
    }
    x0 = star->xpos();
    y0 = star->ypos();
  }

  if (to.level == ScopeLevel::LEVEL_SHIP) {
    const Ship* ship;
    try {
      ship = g.entity_manager.peek_ship(to.shipno);
    } catch (const EntityNotFoundError&) {
      g.out << "Ship not found.\n";
      return false;
    }
    if (ship->owner() != Playernum) {
      g.out << "Nice try.\n";
      return false;
    }
    x1 = ship->xpos();
    y1 = ship->ypos();
  } else if (to.level == ScopeLevel::LEVEL_PLAN) {
    const auto* p = g.entity_manager.peek_planet(to.snum, to.pnum);
    if (!p) {
      g.out << "Planet not found.\n";
      return false;
    }
    const auto* star = g.entity_manager.peek_star(to.snum);
    if (!star) {
      g.out << "Star not found.\n";
      return false;
    }
    x1 = p->xpos() + star->xpos();
    y1 = p->ypos() + star->ypos();
  } else if (to.level == ScopeLevel::LEVEL_STAR) {
    const auto* star = g.entity_manager.peek_star(to.snum);
    if (!star) {
      g.out << "Star not found.\n";
      return false;
    }
    x1 = star->xpos();
    y1 = star->ypos();
  }
  /* compute the distance */
  dist = std::hypot(x0 - x1, y0 - y1);
  g.out << std::format("Distance = {}\n", dist);
  return true;
}

static constexpr std::array<std::string_view, 1> distance_aliases{"dist"};

const CommandDescriptor distance_cmd{
    .name = "distance",
    .aliases = distance_aliases,
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "distance <from> <to>",
    .description = "Calculate distance between stars, planets, or ships",
    .handler = &distance,
};

}  // namespace GB::commands
