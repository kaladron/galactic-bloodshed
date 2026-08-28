// SPDX-License-Identifier: Apache-2.0

export module gblib:order;

import :ships;
import :types;

export void DispOrders(EntityManager& em, player_t Playernum,
                       governor_t Governor, const Ship& ship);
export void DispOrdersHeader(EntityManager& em, player_t Playernum,
                             governor_t Governor);
export void give_orders(GameObj&, const command_t&, int, Ship&);
