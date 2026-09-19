// SPDX-License-Identifier: Apache-2.0

module;

import std;

export module gblib:fuel;

export import gb.entities;
import gb.services;
import std;

export std::tuple<bool, segments_t> do_trip(const Place&, SimulatedShip&,
                                            double fuel, double gravity_factor,
                                            UniverseCoordinates dest_coords,
                                            EntityManager&);

export void fuel_output(GameObj& g, double dist, double fuel, double grav,
                        double mass, segments_t segs,
                        std::string_view plan_buf);
