// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/buffers.h"

module commands;

namespace GB::commands {
void treasury(const command_t &, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  // TODO(jeffbailey): ap_t APcount = 0;
  auto &race = races[Playernum - 1];

  sprintf(
      buf, "Income last update was: %ld\t\tCosts last update was: %ld\n",
      race.governor[Governor].income + race.governor[Governor].profit_market,
      race.governor[Governor].maintain + race.governor[Governor].cost_tech +
          race.governor[Governor].cost_market);
  notify(Playernum, Governor, buf);
  sprintf(buf, "    Market: %5ld\t\t\t     Market: %5ld\n",
          race.governor[Governor].profit_market,
          race.governor[Governor].cost_market);
  notify(Playernum, Governor, buf);
  sprintf(buf, "    Taxes:  %5ld\t\t\t       Tech: %5ld\n",
          race.governor[Governor].income, race.governor[Governor].cost_tech);
  notify(Playernum, Governor, buf);

  sprintf(buf, "\t\t\t\t\t      Maint: %5ld\n",
          race.governor[Governor].maintain);
  notify(Playernum, Governor, buf);
  sprintf(buf, "You have: %ld\n", race.governor[Governor].money);
  notify(Playernum, Governor, buf);
}
}  // namespace GB::commands
