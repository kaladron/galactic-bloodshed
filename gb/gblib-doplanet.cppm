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

export bool moveship_onplanet(Ship& ship, const Planet& planet,
                              EntityManager& entity_manager);
export void terraform(Ship& ship, Planet& planet, SectorMap& smap,
                      EntityManager& entity_manager);
export void plow(Ship* ship, Planet& planet, SectorMap& smap,
                 EntityManager& entity_manager);
export void do_dome(EntityManager& entity_manager, Ship* ship, SectorMap& smap);
export void do_quarry(Ship* ship, Planet& planet, SectorMap& smap,
                      EntityManager& entity_manager, TurnStats& stats);
export void do_recover(EntityManager& entity_manager, const Star& star,
                       Planet& planet);
