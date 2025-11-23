// SPDX-License-Identifier: Apache-2.0

module;

import std;

export module gblib:fuel;

import :place;

export std::tuple<bool, segments_t> do_trip(const Place&, Ship&, double fuel,
                                            double gravity_factor, double x_1,
                                            double y_1);

export void fuel_output(GameObj& g, double dist, double fuel, double grav,
                        double mass, segments_t segs,
                        std::string_view plan_buf);
