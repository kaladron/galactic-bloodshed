// SPDX-License-Identifier: Apache-2.0

export module gblib:build;

import :race;
import :ships;
import :star;
import :types;

export int Shipcost(ShipType, const Race &);
export std::tuple<money_t, double> shipping_cost(starnum_t to, starnum_t from,
                                                 money_t value);
export std::expected<void, std::string> can_build_on_ship(ShipType,
                                                          const Race &,
                                                          const Ship &);
export std::optional<ShipType> get_build_type(char);
export void Getship(Ship *, ShipType, const Race &);
export std::optional<ScopeLevel> build_at_ship(GameObj &g, Ship *builder,
                                               starnum_t *snum,
                                               planetnum_t *pnum);
export void create_ship_by_planet(int, int, const Race &, Ship &, Planet &, int,
                                  int, int, int);
export bool can_build_at_planet(GameObj &g, const Star &star,
                                const Planet &planet);
export std::expected<void, std::string> can_build_this(ShipType what,
                                                       const Race &race);
export std::expected<void, std::string> can_build_on_sector(
    int what, const Race &race, const Planet &planet, const Sector &sector,
    const Coordinates &c);
export int getcount(const command_t &argv, size_t elem);
export void autoload_at_planet(int Playernum, Ship *s, Planet *planet,
                               Sector &sector, int *crew, double *fuel);
export void autoload_at_ship(Ship *s, Ship *b, int *crew, double *fuel);
export void initialize_new_ship(GameObj &g, const Race &race, Ship *newship,
                                double load_fuel, int load_crew);
export void Getfactship(Ship *s, Ship *b);

export void create_ship_by_ship(int Playernum, int Governor, const Race &race,
                                bool outside, Planet *planet, Ship *newship,
                                Ship *builder);
