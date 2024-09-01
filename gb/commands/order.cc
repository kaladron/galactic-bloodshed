// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

#include "gb/order.h"

module commands;

namespace GB::commands {
void order(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;
  shipnum_t shipno;
  shipnum_t nextshipno;
  Ship *ship;

  if (argv.size() == 1) { /* display all ship orders */
    DispOrdersHeader(Playernum, Governor);
    nextshipno = start_shiplist(g, "");
    while ((shipno = do_shiplist(&ship, &nextshipno)))
      if (ship->owner == Playernum && authorized(Governor, *ship)) {
        DispOrders(Playernum, Governor, *ship);
        free(ship);
      } else
        free(ship);
  } else if (argv.size() >= 2) {
    DispOrdersHeader(Playernum, Governor);
    nextshipno = start_shiplist(g, argv[1]);
    while ((shipno = do_shiplist(&ship, &nextshipno)))
      if (in_list(Playernum, argv[1], *ship, &nextshipno) &&
          authorized(Governor, *ship)) {
        if (argv.size() > 2) give_orders(g, argv, APcount, ship);
        DispOrders(Playernum, Governor, *ship);
        free(ship);
      } else
        free(ship);
  } else
    g.out << "I don't understand what you mean.\n";
}
}  // namespace GB::commands
