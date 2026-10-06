// SPDX-License-Identifier: Apache-2.0

/// \file construction.cppm
/// \brief Ship construction validation, factory spawning, and shipping cost
/// mechanics.

export module gb.mechanics:construction;

import gb.entities;
import gb.services;
import std;

export enum class PlanetBuildErrorReason {
  EnslavedByForeignPlayer,
  NotAuthorizedInSystem,
};

export struct PlanetBuildError {
  PlanetBuildErrorReason reason{PlanetBuildErrorReason::NotAuthorizedInSystem};
  player_t enslaving_player{0};
};

export enum class ShipBuildError {
  ShipDead,
  ShipIrradiated,
  NotOwner,
  NotAuthorizedGovernor,
  CannotConstructShips,
  NoCrew,
  ShipDocked,
  ShipDamaged,
  FactoryNotOnline,
  FactoryNotLanded,
};

export struct ShipBuildLocation {
  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  starnum_t snum{0};
  planetnum_t pnum{0};
};

export struct CreatedShipSummary {
  std::string ship_display{};
  resource_t build_cost{0};
  double tech{0.0};
  std::optional<Coordinates> landed_sector{std::nullopt};
  std::optional<Percentage> previous_toxicity{std::nullopt};
  std::optional<Percentage> updated_toxicity{std::nullopt};
};

export struct InitializedShipReport {
  ShipType ship_type{ShipType::STYPE_POD};
  double tele_range{0.0};
  Percentage damage{0};
  bool can_repair{false};
  bool has_crew_capacity{false};
  population_t loaded_crew{0};
  fuel_t loaded_fuel{0.0};
};

export resource_t Shipcost(ShipType, const Race&);
export std::tuple<money_t, double>
shipping_cost(EntityManager& em, starnum_t to, starnum_t from, money_t value);
export std::expected<void, std::string> can_build_on_ship(ShipType, const Race&,
                                                          const Ship&);
export std::optional<ShipType> get_build_type(char);
export std::unique_ptr<Ship> getship(ShipType, const Race&);
export std::expected<ShipBuildLocation, ShipBuildError>
build_at_ship(player_t playernum, governor_t governor, bool god,
              const Ship& builder);
export CreatedShipSummary create_ship_by_planet(EntityManager& entity_manager,
                                                player_t, governor_t,
                                                const Race&, Ship&, Planet&,
                                                starnum_t, planetnum_t,
                                                Coordinates land_coords);
export std::expected<void, PlanetBuildError>
can_build_at_planet(player_t playernum, governor_t governor, const Star& star,
                    const Planet& planet);
export std::expected<void, std::string> can_build_this(ShipType what,
                                                       const Race& race);
export std::expected<void, std::string>
can_build_on_sector(EntityManager& entity_manager, ShipType what,
                    const Race& race, const Planet& planet,
                    const Sector& sector, const Coordinates& c);
export int getcount(const command_t& argv, std::size_t elem);
export std::pair<population_t, fuel_t> autoload_at_planet(player_t Playernum,
                                                          const Ship& s,
                                                          Planet& planet,
                                                          Sector& sector);
export std::pair<population_t, fuel_t> autoload_at_ship(const Ship& s, Ship& b,
                                                        double race_mass = 1.0);
export InitializedShipReport
initialize_new_ship(const Race& race, governor_t governor, Ship& newship,
                    fuel_t load_fuel, population_t load_crew);
export std::unique_ptr<Ship> getfactship(const Ship& b);

export CreatedShipSummary create_ship_by_ship(EntityManager& entity_manager,
                                              player_t Playernum,
                                              governor_t Governor,
                                              const Race& race, bool outside,
                                              Ship& newship, Ship& builder);
