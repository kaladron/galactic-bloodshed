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

  if (argv.size() == 1) { /* display all ship orders */
    DispOrdersHeader(Playernum, Governor);
    const ShipList kShips(g.entity_manager, g, ShipList::IterationType::Scope);
    for (const Ship* ship : kShips) {
      if (ship->owner() == Playernum && authorized(Governor, *ship)) {
        DispOrders(g.entity_manager, Playernum, Governor, *ship);
      }
    }
  } else if (argv.size() >= 2) {
    DispOrdersHeader(Playernum, Governor);
    ShipList ships(g.entity_manager, g, ShipList::IterationType::Scope);
    for (auto ship_handle : ships) {
      Ship& ship = *ship_handle;

      if (!ship_matches_filter(argv[1], ship)) continue;
      if (!authorized(Governor, ship)) continue;

      if (argv.size() > 2) {
        give_orders(g, argv, APcount, ship);
      }

      DispOrders(g.entity_manager, Playernum, Governor, ship);

      // Early exit for specific ship number filters
      if (is_ship_number_filter(argv[1])) break;
    }
  } else
    g.out << "I don't understand what you mean.\n";
}
}  // namespace GB::commands
