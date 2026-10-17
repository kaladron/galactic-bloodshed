// SPDX-License-Identifier: Apache-2.0

/// \file capital.cc
/// \brief Designate a capital.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool capital(const command_t& argv, GameObj& g) {
  constexpr ap_t CAPITAL_AP_COST = 50;

  std::optional<shipnum_t> shipno;
  if (argv.size() != 2) {
    shipno = g.race->Gov_ship;
  } else {
    shipno = string_to_shipnum(argv[1]);
    if (!shipno) {
      g.out << "Specify a valid ship number.\n";
      return false;
    }
  }

  if (!shipno) {
    g.out << "Change the capital to be what ship?\n";
    return false;
  }

  try {
    return g.entity_manager.with_ship(*shipno, [&](const Ship& s) {
      if (argv.size() == 2) {
        starnum_t snum = s.storbits();
        if (!g.check_commandable(s)) {
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

        if (!g.deduct_ap(snum, CAPITAL_AP_COST)) {
          g.out << std::format("You don't have {} action points there.\n",
                               CAPITAL_AP_COST);
          return false;
        }

        g.entity_manager.mutate_race(
            g.player(), [&](Race& race) { race.Gov_ship = shipno; });
      }

      const double efficiency = (static_cast<double>(s.popn()) /
                                 static_cast<double>(s.max_crew_capacity())) *
                                (100.0 - static_cast<double>(s.damage()));
      g.present(GB::presentation::CapitalResult{
          .shipno = *shipno,
          .designated = (argv.size() == 2),
          .efficiency = efficiency,
      });
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
