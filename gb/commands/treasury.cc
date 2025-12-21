// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std;

module commands;

namespace GB::commands {
void treasury(const command_t&, GameObj& g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;

  g.out << std::format(
      "Income last update was: {}\t\tCosts last update was: {}\n",
      g.race->governor[Governor].income +
          g.race->governor[Governor].profit_market,
      g.race->governor[Governor].maintain +
          g.race->governor[Governor].cost_tech +
          g.race->governor[Governor].cost_market);
  g.out << std::format("    Market: {:5}\t\t\t     Market: {:5}\n",
                       g.race->governor[Governor].profit_market,
                       g.race->governor[Governor].cost_market);
  g.out << std::format("    Taxes:  {:5}\t\t\t       Tech: {:5}\n",
                       g.race->governor[Governor].income,
                       g.race->governor[Governor].cost_tech);
  g.out << std::format("\t\t\t\t\t      Maint: {:5}\n",
                       g.race->governor[Governor].maintain);
  g.out << std::format("You have: {}\n", g.race->governor[Governor].money);
}
}  // namespace GB::commands
