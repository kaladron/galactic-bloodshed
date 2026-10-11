// SPDX-License-Identifier: Apache-2.0

/// \file examine.cc
/// \brief Examine an entity or object in detail.

module;

import std;
import gb.entities;
import gb.presentation;
import gb.services;

module commands;

namespace GB::commands {
bool examine(const command_t& argv, GameObj& g) {
  const ap_t APcount = 0;

  auto shipno = string_to_shipnum(argv[1]);
  if (!shipno) {
    return false;
  }

  const Ship* ship = nullptr;
  try {
    ship = g.entity_manager.peek_ship(*shipno);
  } catch (const EntityNotFoundError&) {
    g.out << "Ship not found.\n";
    return false;
  }

  if (ship->whatorbits() == ScopeLevel::LEVEL_UNIV) {
    g.out << "That ship it not visible to you.\n";
    return false;
  }
  const bool visible =
      g.entity_manager.with_star(ship->storbits(), [&](const Star& star) {
        return star.is_inhabited_by(g.player());
      });
  if (!visible) {
    g.out << "That ship it not visible to you.\n";
    return false;
  }

  GB::presentation::ExamineViewModel vm{
      .shipno = *shipno,
      .ship_type = ship->type(),
      .has_switch = ship->has_switch(),
      .irradiated = !ship->active(),
  };
  g.entity_manager.with_ship_exam(ship->type(), [&](const ShipExam& exam) {
    vm.description = exam.description;
  });

  if (!ship->examined()) {
    g.deduct_ap(ship->storbits(), APcount);
    g.entity_manager.mutate_ship(*shipno, [](Ship& s) { s.examined() = 1; });
  }

  g.out << GB::presentation::render_for_mode(g.ui_mode(), vm);
  return true;
}

const CommandDescriptor examine_cmd{
    .name = "examine",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 2,
    .syntax = "examine <#shipnum>",
    .description = "Examine a ship or object in detail",
    .handler = &examine,
};

}  // namespace GB::commands
