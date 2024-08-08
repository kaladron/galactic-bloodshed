// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef BUILD_H
#define BUILD_H

int Shipcost(ShipType, const Race &);
std::tuple<money_t, double> shipping_cost(starnum_t to, starnum_t from,
                                          money_t value);
bool can_build_on_ship(int, const Race &, Ship *, char *);
std::optional<ShipType> get_build_type(const char);
std::optional<ScopeLevel> build_at_ship(GameObj &, Ship *, int *, int *);
void Getship(Ship *, ShipType, const Race &);
std::optional<ScopeLevel> build_at_ship(GameObj &g, Ship *builder, int *snum,
                                        int *pnum);
void create_ship_by_planet(int, int, const Race &, Ship &, Planet &, int, int,
                           int, int);
bool can_build_at_planet(GameObj &g, const Star &star, const Planet &planet);
bool can_build_this(const ShipType what, const Race &race, char *string);
bool can_build_on_sector(const int what, const Race &race, const Planet &planet,
                         const Sector &sector, const int x, const int y,
                         char *string);
int getcount(const command_t &argv, const size_t elem);
void autoload_at_planet(int Playernum, Ship *s, Planet *planet, Sector &sector,
                        int *crew, double *fuel);
void autoload_at_ship(Ship *s, Ship *b, int *crew, double *fuel);
void initialize_new_ship(GameObj &g, const Race &race, Ship *newship,
                         double load_fuel, int load_crew);
void Getfactship(Ship *s, Ship *b);

void create_ship_by_ship(int Playernum, int Governor, const Race &race,
                         int outside, Planet *planet, Ship *newship,
                         Ship *builder);

extern const char *commod_name[4];

#endif  // BUILD_H
