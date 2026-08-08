// SPDX-License-Identifier: Apache-2.0

/// \file examine.cc
/// \brief Examine an entity or object in detail.

module;

#include "gb/files.h"

import std;
import gblib;

module commands;

namespace GB::commands {
void examine(const command_t& argv, GameObj& g) {
  const ap_t APcount = 0;
  std::FILE* fd;
  int ch;

  if (argv.size() < 2) {
    g.out << "Examine what?\n";
    return;
  }

  auto shipno = string_to_shipnum(argv[1]);

  if (!shipno) {
    return;
  }

  try {
    g.entity_manager.peek_ship(*shipno);
  } catch (const EntityNotFoundError&) {
    g.out << "Ship not found.\n";
    return;
  }
  auto ship = g.entity_manager.get_ship(*shipno);

  if (!ship->alive()) {
    g.out << "that ship is dead.\n";
    return;
  }
  if (ship->whatorbits() == ScopeLevel::LEVEL_UNIV) {
    g.out << "That ship it not visible to you.\n";
    return;
  }
  const auto& star = *g.entity_manager.peek_star(ship->storbits());
  if (isclr(star.inhabited(), g.player())) {
    g.out << "That ship it not visible to you.\n";
    return;
  }

  if ((fd = std::fopen(EXAM_FL, "r")) == nullptr) {
    std::perror(EXAM_FL);
    return;
  }

  /* look through ship data file */
  for (int t = 0; t <= ship->type(); t++)
    while (std::fgetc(fd) != '~')
      ;

  /* look through ship data file */
  g.out << "\n";
  /* give report */
  std::stringstream ss;
  while ((ch = std::fgetc(fd)) != '~' && ch != -1) {
    ss << static_cast<char>(ch);
  }
  g.out << ss.str();
  std::fclose(fd);

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
