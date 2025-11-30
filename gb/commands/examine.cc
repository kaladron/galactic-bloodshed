// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/* examine -- check out an object */

module;

import gblib;
import std.compat;

#include "gb/files.h"

module commands;

namespace GB::commands {
void examine(const command_t& argv, GameObj& g) {
  const ap_t APcount = 0;
  FILE* fd;
  char ch;

  if (argv.size() < 2) {
    g.out << "Examine what?\n";
    return;
  }

  auto shipno = string_to_shipnum(argv[1]);

  if (!shipno) {
    return;
  }

  auto ship = g.entity_manager.get_ship(*shipno);

  if (!ship.get()) {
    g.out << "Ship not found.\n";
    return;
  }

  if (!ship->alive()) {
    g.out << "that ship is dead.\n";
    return;
  }
  if (ship->whatorbits() == ScopeLevel::LEVEL_UNIV ||
      isclr(stars[ship->storbits()].inhabited(), g.player)) {
    g.out << "That ship it not visible to you.\n";
    return;
  }

  if ((fd = fopen(EXAM_FL, "r")) == nullptr) {
    perror(EXAM_FL);
    return;
  }

  /* look through ship data file */
  for (int t = 0; t <= ship->type(); t++)
    while (fgetc(fd) != '~')
      ;

  /* look through ship data file */
  g.out << "\n";
  /* give report */
  std::stringstream ss;
  while ((ch = fgetc(fd)) != '~') {
    ss << ch;
  }
  g.out << ss.str();
  fclose(fd);

  if (!ship->examined()) {
    if (ship->whatorbits() == ScopeLevel::LEVEL_UNIV)
      deductAPs(g, APcount, ScopeLevel::LEVEL_UNIV);
    else
      deductAPs(g, APcount, ship->storbits());

    ship->examined() = 1;
  }

  if (has_switch(*ship)) {
    g.out << "This device has an on/off switch that can be set with order.\n";
  }
  if (!ship->active()) {
    g.out << "This device has been irradiated;\n";
    g.out << "Its crew is dying and it cannot move for the time being.\n";
  }
}
}  // namespace GB::commands
