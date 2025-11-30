// Copyright 2019 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void distance(const command_t& argv, GameObj& g) {
  const player_t Playernum = g.player;
  const governor_t Governor = g.governor;
  double x0;
  double y0;
  double x1;
  double y1;
  double dist;

  if (argv.size() < 3) {
    g.out << "Syntax: 'distance <from> <to>'.\n";
    return;
  }

  Place from{g, argv[1], true};
  if (from.err) {
    notify(Playernum, Governor, std::format("Bad scope '{}'\n", argv[1]));
    return;
  }
  Place to{g, argv[2], true};
  if (to.err) {
    notify(Playernum, Governor, std::format("Bad scope '{}'\n", argv[2]));
  }

  x0 = 0.0;
  y0 = 0.0;
  x1 = 0.0;
  y1 = 0.0;
  /* get position in absolute units */
  if (from.level == ScopeLevel::LEVEL_SHIP) {
    const auto* ship = g.entity_manager.peek_ship(from.shipno);
    if (!ship) {
      g.out << "Ship not found.\n";
      return;
    }
    if (ship->owner() != Playernum) {
      g.out << "Nice try.\n";
      return;
    }
    x0 = ship->xpos();
    y0 = ship->ypos();
  } else if (from.level == ScopeLevel::LEVEL_PLAN) {
    const auto* p = g.entity_manager.peek_planet(from.snum, from.pnum);
    if (!p) {
      g.out << "Planet not found.\n";
      return;
    }
    const auto* star = g.entity_manager.peek_star(from.snum);
    if (!star) {
      g.out << "Star not found.\n";
      return;
    }
    x0 = p->xpos() + star->xpos();
    y0 = p->ypos() + star->ypos();
  } else if (from.level == ScopeLevel::LEVEL_STAR) {
    const auto* star = g.entity_manager.peek_star(from.snum);
    if (!star) {
      g.out << "Star not found.\n";
      return;
    }
    x0 = star->xpos();
    y0 = star->ypos();
  }

  if (to.level == ScopeLevel::LEVEL_SHIP) {
    const auto* ship = g.entity_manager.peek_ship(to.shipno);
    if (!ship) {
      g.out << "Ship not found.\n";
      return;
    }
    if (ship->owner() != Playernum) {
      g.out << "Nice try.\n";
      return;
    }
    x1 = ship->xpos();
    y1 = ship->ypos();
  } else if (to.level == ScopeLevel::LEVEL_PLAN) {
    const auto* p = g.entity_manager.peek_planet(to.snum, to.pnum);
    if (!p) {
      g.out << "Planet not found.\n";
      return;
    }
    const auto* star = g.entity_manager.peek_star(to.snum);
    if (!star) {
      g.out << "Star not found.\n";
      return;
    }
    x1 = p->xpos() + star->xpos();
    y1 = p->ypos() + star->ypos();
  } else if (to.level == ScopeLevel::LEVEL_STAR) {
    const auto* star = g.entity_manager.peek_star(to.snum);
    if (!star) {
      g.out << "Star not found.\n";
      return;
    }
    x1 = star->xpos();
    y1 = star->ypos();
  }
  /* compute the distance */
  dist = sqrt(Distsq(x0, y0, x1, y1));
  notify(Playernum, Governor, std::format("Distance = {}\n", dist));
}
}  // namespace GB::commands
