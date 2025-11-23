// SPDX-License-Identifier: Apache-2.0

module;

import gblib;
import std.compat;

module commands;

namespace GB::commands {
void order(const command_t& argv, GameObj& g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  ap_t APcount = 1;
  shipnum_t nextshipno = 0;  // Used by in_list() to signal early termination

  if (argv.size() == 1) { /* display all ship orders */
    DispOrdersHeader(Playernum, Governor);
    shipnum_t n_ships = g.entity_manager.num_ships();
    for (shipnum_t i = 1; i <= n_ships; i++) {
      const auto* ship = g.entity_manager.peek_ship(i);
      if (!ship || !ship->alive) continue;
      if (ship->owner == Playernum && authorized(Governor, *ship)) {
        DispOrders(Playernum, Governor, *ship);
      }
    }
  } else if (argv.size() >= 2) {
    DispOrdersHeader(Playernum, Governor);
    shipnum_t n_ships = g.entity_manager.num_ships();
    for (shipnum_t i = 1; i <= n_ships; i++) {
      const auto* ship = g.entity_manager.peek_ship(i);
      if (!ship || !ship->alive) continue;
      if (in_list(Playernum, argv[1], *ship, &nextshipno) &&
          authorized(Governor, *ship)) {
        if (argv.size() > 2) {
          {
            // Get mutable handle for modifications
            auto ship_handle = g.entity_manager.get_ship(i);
            if (ship_handle.get()) {
              give_orders(g, argv, APcount, *ship_handle);
              // Auto-saves when ship_handle goes out of scope
            }
          }  // ship_handle destructor runs here, may evict from cache
          // Must peek again after EntityHandle destruction to get valid pointer
          ship = g.entity_manager.peek_ship(i);
          if (!ship) continue;
        }
        DispOrders(Playernum, Governor, *ship);

        // in_list sets nextshipno=0 for #N selectors to signal early exit
        if (nextshipno == 0) break;
      }
    }
  } else
    g.out << "I don't understand what you mean.\n";
}
}  // namespace GB::commands
