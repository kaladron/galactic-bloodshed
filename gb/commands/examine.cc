// SPDX-License-Identifier: Apache-2.0

/// \file examine.cc
/// \brief Examine an entity or object in detail.

module;

import std;
import gblib;

module commands;

namespace GB::commands {
bool examine(const command_t& argv, GameObj& g) {
  const ap_t APcount = 0;

  if (argv.size() < 2) {
    g.out << "Examine what?\n";
    return false;
  }

  auto shipno = string_to_shipnum(argv[1]);

  if (!shipno) {
    return false;
  }

  try {
    bool ok = false;
    g.entity_manager.mutate_ship(*shipno, [&](Ship& ship) {
      if (!ship.alive()) {
        g.out << "that ship is dead.\n";
        return;
      }
      if (ship.whatorbits() == ScopeLevel::LEVEL_UNIV) {
        g.out << "That ship it not visible to you.\n";
        return;
      }
      bool visible =
          g.entity_manager.with_star(ship.storbits(), [&](const Star& star) {
            return isset(star.inhabited(), g.player());
          });
      if (!visible) {
        g.out << "That ship it not visible to you.\n";
        return;
      }

      g.entity_manager.with_ship_exam(ship.type(), [&](const ShipExam& exam) {
        if (!exam.description.empty()) {
          g.out << "\n" << exam.description;
          if (!exam.description.ends_with('\n')) {
            g.out << "\n";
          }
        }
      });

      if (!ship.examined()) {
        if (ship.whatorbits() == ScopeLevel::LEVEL_UNIV) {
          g.deduct_univ_ap(APcount);
        } else {
          g.deduct_ap(ship.storbits(), APcount);
        }

        ship.examined() = 1;
      }

      if (has_switch(ship)) {
        g.out
            << "This device has an on/off switch that can be set with order.\n";
      }
      if (!ship.active()) {
        g.out << "This device has been irradiated;\n";
        g.out << "Its crew is dying and it cannot move for the time being.\n";
      }
      ok = true;
    });
    return ok;
  } catch (const EntityNotFoundError&) {
    g.out << "Ship not found.\n";
    return false;
  }
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
