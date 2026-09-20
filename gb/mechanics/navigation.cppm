// SPDX-License-Identifier: Apache-2.0

/// \file navigation.cppm
/// \brief Ship navigation, sublight/hyperdrive movement, trip simulation, and
/// ship order mechanics.

module;

export module gb.mechanics:navigation;

import gb.entities;
import gb.services;
import std;

export armor_t getdefense(EntityManager&, const Ship&);
export void capture_stuff(const Ship&, GameObj&);
export void domass(Ship&, EntityManager&);
export void doown(Ship&, EntityManager&);
export std::string prin_ship_orbits(EntityManager&, const Ship&);
export std::string format_ship_dest(EntityManager&, const Ship&);
export void moveship(EntityManager&, Ship& ship, bool is_update,
                     bool send_messages, bool checking_fuel);
export void msg_OOF(EntityManager&, const Ship& ship);
export bool followable(EntityManager&, const Ship& ship, const Ship& target);
export std::string dispshiploc_brief(EntityManager&, const Ship&);
export std::string dispshiploc(EntityManager&, const Ship&);

export std::tuple<bool, segments_t> do_trip(const Place&, SimulatedShip&,
                                            double fuel, double gravity_factor,
                                            UniverseCoordinates dest_coords,
                                            EntityManager&);

export void fuel_output(GameObj& g, double dist, double fuel, double grav,
                        double mass, segments_t segs,
                        std::string_view plan_buf);

export void display_orders(GameObj& g, const Ship& ship);
export void display_orders_header(GameObj& g);
export void give_orders(GameObj&, const command_t&, int, Ship&);
