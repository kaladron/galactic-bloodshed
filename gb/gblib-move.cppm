// SPDX-License-Identifier: Apache-2.0

export module gblib:move;

import :planet;
import :sector;
import :services;
import :ships;
import :types;

export Coordinates get_move(const Planet& planet, char direction,
                            Coordinates from);

export void ground_attack(Race&, Race&, int*, PopulationType, population_t*,
                          population_t*, unsigned int, unsigned int, double,
                          double, double*, double*, int*, int*, int*);

export void mech_defend(const GameObj& g, int* people, PopulationType what,
                        const Planet& p, int x2, int y2, const Sector& s2);

export void mech_attack_people(EntityManager& em, Ship& ship, population_t* civ,
                               population_t* mil, Race& race, Race& alien,
                               const Sector& sect, bool ignore, char* long_msg,
                               char* short_msg);

export void people_attack_mech(EntityManager& em, Ship& ship, int civ, int mil,
                               Race& race, Race& alien, const Sector& sect,
                               int x, int y, char* long_msg, char* short_msg);
