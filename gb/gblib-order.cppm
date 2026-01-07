// SPDX-License-Identifier: Apache-2.0

export module gblib:order;

import :ships;
import :types;

export void DispOrders(EntityManager& em, int Playernum, int Governor,
                       const Ship& ship);
export void DispOrdersHeader(EntityManager& em, int Playernum, int Governor);
export void give_orders(GameObj&, const command_t&, int, Ship&);
