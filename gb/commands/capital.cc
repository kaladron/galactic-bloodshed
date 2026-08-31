// SPDX-License-Identifier: Apache-2.0

/// \file capital.cc
/// \brief Designate a capital.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool capital(const command_t& argv, GameObj& g) {
  const ap_t kAPCost = 50;

  shipnum_t shipno = 0;
  if (argv.size() != 2) {
    shipno = g.race->Gov_ship;
  } else {
    auto shiptmp = string_to_shipnum(argv[1]);
    if (!shiptmp) {
      g.out << "Specify a valid ship number.\n";
      return false;
    }
    shipno = *shiptmp;
  }

  if (shipno == 0) {
    g.out << "Change the capital to be what ship?\n";
    return false;
  }

  try {
    return g.entity_manager.with_ship(shipno, [&](const Ship& s) {
      if (argv.size() == 2) {
        starnum_t snum = s.storbits();
        if (testship(s, g)) {
          g.out << "You can't do that!\n";
          return false;
        }
        if (!s.is_landed()) {
          g.out << "Try landing this ship first!\n";
          return false;
        }

        if (s.type() != ShipType::OTYPE_GOV) {
          g.out << std::format("That ship is not a {}.\n",
                               ship_template(ShipType::OTYPE_GOV).name);
          return false;
        }

        if (!g.deduct_ap(snum, kAPCost)) {
          g.out << std::format("You don't have {} action points there.\n",
                               kAPCost);
          return false;
        }

        g.entity_manager.mutate_race(
            g.player(), [&](Race& race) { race.Gov_ship = shipno; });
      }

      g.out << std::format("Efficiency of governmental center: {:.0f}%.\n",
                           ((double)s.popn() / (double)s.max_crew_capacity()) *
                               (100 - (double)s.damage()));
      return true;
    });
  } catch (const EntityNotFoundError&) {
    g.out << "Change the capital to be what ship?\n";
    return false;
  }
}

const CommandDescriptor capital_cmd{
    .name = "capital",
    .roles = {.leader_only = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 1,
    .syntax = "capital [<ship>]",
    .description = "Query or designate your governmental center (capital ship)",
    .handler = &capital,
};

}  // namespace GB::commands
