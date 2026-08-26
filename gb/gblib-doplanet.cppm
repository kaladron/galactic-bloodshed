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

/// \brief Updates planetary temperature and climate based on space mirror
/// warming/cooling trends.
export void process_planet_climate(Planet& planet, const Star& star,
                                   const TurnStats& stats);

/// \brief If planetary toxicity exceeds ENVIR_DAMAGE_TOX, devastates a random
/// sector and returns the devastated coordinates, or std::nullopt if no damage
/// occurred.
export std::optional<Coordinates>
process_toxic_environmental_damage(const Planet& planet, SectorMap& smap);

/// \brief If star is undergoing supernova, applies radiation devastation
/// across all inhabited sectors. Returns true if any inhabited sectors were
/// affected.
export bool process_supernova_sector_devastation(const Star& star,
                                                 SectorMap& smap);

/// \brief If automated waste canister threshold is set and conditions met,
/// builds a toxic waste canister ship, reduces planetary toxicity, and places
/// the ship on the planet. Returns the new ship number if constructed.
export std::optional<shipnum_t>
build_automated_waste_can(EntityManager& entity_manager, const Star& star,
                          Planet& planet, SectorMap& smap, const Race& race);

/// \brief Verifies that all players in the provided list are mutually allied.
/// Returns true if the list contains 0 or 1 player, or if every distinct pair
/// of players has mutual alliance bits set. Returns false if any race cannot be
/// loaded or if any pair is not mutually allied.
export bool check_mutual_alliances(EntityManager& entity_manager,
                                   std::span<const player_t> players);

export enum class PlunderError {
  NoConquerors,
  EmptyLoot,
};

export struct PlayerLootShare {
  player_t player{0};
  Stockpile share{};

  [[nodiscard]] bool
  operator==(const PlayerLootShare&) const noexcept = default;
};

export struct PlunderDistribution {
  std::vector<PlayerLootShare> shares;
  Stockpile total_loot{};

  [[nodiscard]] bool
  operator==(const PlunderDistribution&) const noexcept = default;
};

/// \brief Divvies up a looted stockpile among conquerors.
/// The first (N - 1) conquerors receive their rounded share, while the final
/// conqueror receives all exact remaining leftovers, ensuring zero loss or
/// creation of commodities. Returns PlunderDistribution on success, or
/// PlunderError if conquerors list is empty or loot is empty.
export std::expected<PlunderDistribution, PlunderError>
calculate_plunder_distribution(Stockpile total_loot,
                               std::span<const player_t> conquerors);

export void do_recover(EntityManager& entity_manager, const Star& star,
                       Planet& planet);
