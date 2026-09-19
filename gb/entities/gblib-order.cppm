// SPDX-License-Identifier: Apache-2.0

export module gblib:order;

import gb.entities;
import gb.services;
import std;

export void display_orders(GameObj& g, const Ship& ship);
export void display_orders_header(GameObj& g);
export void give_orders(GameObj&, const command_t&, int, Ship&);
