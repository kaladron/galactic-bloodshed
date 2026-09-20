// SPDX-License-Identifier: Apache-2.0

/// \file vn.cppm
/// \brief Module interface partition for autonomous Von Neumann and Berserker
/// turn processing.

export module gb.turn:vn;

import gb.entities;
import gb.services;
import :turnstats;
import std;

/// \brief Top two nearest star systems identified by navigation scanning.
export struct StarTargetResult {
  starnum_t closest{0};         ///< Primary nearest star system
  starnum_t second_closest{0};  ///< Secondary nearest star system
};

/// \brief Finds the closest and second-closest star systems to the given
/// coordinates, excluding the current star system.
export StarTargetResult find_closest_stars(EntityManager& em,
                                           starnum_t current_star,
                                           UniverseCoordinates origin);

/// \brief Assigns destination orders to an autonomous berserker ship.
export void select_berserker_destination(EntityManager& em,
                                         AutonomousShip& ship,
                                         const TurnStats& stats);

/// \brief Assigns destination orders to an autonomous Von Neumann machine.
export void select_vn_destination(EntityManager& em, AutonomousShip& ship);

/// \brief Orders an autonomous Berserker to select a target and destination.
export void order_berserker(EntityManager& em, Ship& ship, TurnStats& stats);

/// \brief Orders an autonomous Von Neumann machine to select a destination.
export void order_VN(EntityManager& em, Ship& ship);

/// \brief Performs turn processing for an autonomous Von Neumann or Berserker
/// machine.
export void do_VN(EntityManager& em, AutonomousShip& ship, TurnStats& stats);

/// \brief Performs planetary surface operations (landing, mining, replication)
/// for an autonomous machine.
export void planet_doVN(Ship& ship, Planet& planet, SectorMap& smap,
                        EntityManager& entity_manager, TurnStats& stats);

/// \brief Result of stealing planetary resources from an alien colony.
export struct StealResult {
  player_t victim{0};    ///< Player ID victimized, or 0 if none
  resource_t amount{0};  ///< Quantity of resources stolen
};

/// \brief Steals resources from alien colonies on the currently landed planet.
export StealResult steal_planetary_resources(EntityManager& em,
                                             AutonomousShip& ship);

/// \brief Constructs and deploys a newly replicated Von Neumann machine on a
/// planet.
export shipnum_t construct_replicated_vn(EntityManager& em,
                                         AutonomousShip& parent,
                                         Planet& planet);

/// \brief Constructs and deploys a newly constructed Berserker warship on a
/// planet.
export shipnum_t construct_replicated_berserker(EntityManager& em,
                                                AutonomousShip& parent,
                                                Planet& planet,
                                                const TurnStats& stats);

/// \brief Replicates as many autonomous machines as parent resources allow.
export int replicate_machines(EntityManager& em, AutonomousShip& parent,
                              Planet& planet, const TurnStats& stats);

/// \brief Attempts to launch an unassigned, fully fueled Von Neumann machine
/// into deep space.
export bool try_launch_unassigned_vn(EntityManager& em, AutonomousShip& ship);

/// \brief Attempts to land an orbiting autonomous machine onto a
/// resource-bearing planetary sector.
export bool attempt_planet_landing(EntityManager& em, AutonomousShip& ship,
                                   const Planet& planet, SectorMap& smap);
