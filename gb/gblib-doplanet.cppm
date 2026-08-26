// SPDX-License-Identifier: Apache-2.0

export module gblib:doplanet;

import std;
import :planet;
import :services;
import :star;
import :turnstats;
import :types;

import :ships;

export int doplanet(EntityManager&, const Star& star, Planet& planet,
                    TurnStats& stats);

export void moveplanet(EntityManager& entity_manager, const Star& star,
                       Planet& planet);

export enum class GroundMovementError {
  NotTerraformVehicle,
  Stopped,
  EmptyOrders,
  InvalidIndex,
};

export std::expected<char, GroundMovementError>
get_ground_order(const Ship& ship, std::size_t index);

export std::expected<Coordinates, GroundMovementError>
advance_ground_vehicle(Ship& ship, const Planet& planet,
                       EntityManager& entity_manager);

export enum class GroundActionError {
  NotSwitchedOn,
  NotLanded,
  NoCrew,
  InsufficientFuel,
  InsufficientResources,
  MovementFailed,
  SectorAlreadyOptimal,
  IncompatibleSector,
};

export bool moveship_onplanet(Ship& ship, const Planet& planet,
                              EntityManager& entity_manager);

/// \brief Moves terraformer vehicle and attempts to convert target sector
/// condition. Returns whether terraforming succeeded, or GroundActionError.
export std::expected<bool, GroundActionError>
execute_terraforming(Ship& ship, Planet& planet, SectorMap& smap,
                     EntityManager& entity_manager);

/// \brief Moves space plow and improves target sector fertility.
/// Returns fertility increase amount, or GroundActionError.
export std::expected<int, GroundActionError>
execute_plowing(Ship& ship, Planet& planet, SectorMap& smap,
                EntityManager& entity_manager);

/// \brief Upgrades constructor dome efficiency using ship resources.
/// Returns efficiency increase amount, or GroundActionError.
export std::expected<int, GroundActionError>
upgrade_sector_dome(EntityManager& entity_manager, Ship& ship, SectorMap& smap);

/// \brief Strip mines quarry sector, producing resources and generating
/// pollution. Returns resources produced, or GroundActionError.
export std::expected<int, GroundActionError>
strip_mine_quarry(Ship& ship, Planet& planet, SectorMap& smap,
                  EntityManager& entity_manager, TurnStats& stats);

/// \brief Executes berserker bombardment on target planet if in orbit.
/// Decrements VN hitlist on kill or selects next destination planet if no
/// targets found. Returns true if bombardment caused destruction, false
/// otherwise.
export bool execute_berserker_bombardment(EntityManager& entity_manager,
                                          Ship& ship, Planet& planet);

/// \brief Refuels ships in orbit around a gas giant planet based on ship type
/// capacity. Returns amount of fuel added (0.0 if not in orbit or not a gas
/// giant).
export double refuel_gasgiant_orbiters(const Planet& planet, Ship& ship);

/// \brief Processes all planetary ships (VN replication, berserker bombardment,
/// terraforming, plowing, dome construction, weapon plants, quarrying, and gas
/// refueling).
export void process_planetary_ships(EntityManager& entity_manager,
                                    Planet& planet, SectorMap& smap,
                                    TurnStats& stats);

export void do_recover(EntityManager& entity_manager, const Star& star,
                       Planet& planet);
