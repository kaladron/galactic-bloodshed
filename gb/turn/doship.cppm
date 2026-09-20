// SPDX-License-Identifier: Apache-2.0

/// \file doship.cppm
/// \brief Module interface partition for ship turn simulation processing.

export module gb.turn:doship;

import gb.entities;
import gb.services;
import :turnstats;
import std;

export void doship(Ship&, bool update, EntityManager&, TurnStats& stats);
/// \brief Simulates turn-phase missile navigation, PDN interception, and
/// terminal impacts against planetary surfaces or targeted ships.
/// \param ship Missile ship executing turn processing.
/// \param entity_manager Entity manager for spatial queries and mutations.
export void domissile(Ship& ship, EntityManager& entity_manager);

/// \brief Checks if planetary defense nodes (PDN) are present on the target
/// planet and redirects the missile to attack the PDN if found.
/// \param missile Missile ship executing attack.
/// \param entity_manager Entity manager for scoped ship queries.
/// \return True if intercepted and redirected to a PDN, false otherwise.
export bool intercept_missile_by_pdn(Ship& missile,
                                     EntityManager& entity_manager);

/// \brief Executes a missile impact strike against a planet's surface,
/// resolving damage against either designated target coordinates (with toroidal
/// wrapping) or scattered random sectors.
/// \param missile Missile ship executing planetary bombardment.
/// \param entity_manager Entity manager for planet, sectormap, and ship
/// mutations.
export void execute_missile_planet_strike(Ship& missile,
                                          EntityManager& entity_manager);

/// \brief Executes a ship-to-ship missile strike against its destination ship
/// if within effective strike distance.
/// \param missile Missile ship executing the strike.
/// \param entity_manager Entity manager for target mutation and notifications.
export void execute_missile_ship_strike(Ship& missile,
                                        EntityManager& entity_manager);

/// \brief Simulates surface-to-orbit anti-ballistic missile (ABM) defenses
/// intercepting incoming hostile missiles and mines.
/// \param ship ABM defense ship landed on planetary surface.
/// \param entity_manager Entity manager for querying orbital threats and
/// attacking.
export void doabm(Ship& ship, EntityManager& entity_manager);

/// \brief Manufactures destructive ordnance on weapon plant ships from mineral
/// and propellant stockpiles.
/// \param ship Weapon plant ship.
/// \param entity_manager Entity manager for owner race lookups.
/// \return Quantity of destructive charges produced.
export int do_weapon_plant(Ship& ship, EntityManager& entity_manager);

/// \brief Executes automated or crewed damage repair and resource consumption.
/// \param ship Ship undergoing maintenance.
/// \param entity_manager Entity manager for docked stations and state.
export void do_repair(Ship& ship, EntityManager& entity_manager);

/// \brief Synthesizes resources from propellant, breeds colonists, and triggers
/// nested weapon plants inside orbital habitats.
/// \param ship Habitat ship.
/// \param entity_manager Entity manager for race and nested ship queries.
export void do_habitat(Ship& ship, EntityManager& entity_manager);
/// \brief Simulates spore pod warming, detonation, and planetary meta-colony
/// seeding.
export void do_pod(SporePodShip& ship, EntityManager& entity_manager);

/// \brief Simulates dust canister atmospheric cooling and eventual dissipation.
export void do_canister(CanisterShip& ship, EntityManager& entity_manager,
                        TurnStats& stats);

/// \brief Simulates greenhouse gas warming and eventual dissipation.
export void do_greenhouse(CanisterShip& ship, EntityManager& entity_manager,
                          TurnStats& stats);

/// \brief Simulates orbital assault platform intimidation of the planet below.
export void do_oap(Ship& ship, TurnStats& stats);

/// \brief Simulates focused space mirror heating against ships, planets, or
/// stars.
export void do_mirror(SpaceMirrorShip& ship, EntityManager& entity_manager,
                      TurnStats& stats);
export void do_meta_infect(player_t who, starnum_t star, planetnum_t pnum,
                           Planet& p, EntityManager& entity_manager);
export int infect_planet(player_t who, starnum_t star, planetnum_t pnum,
                         EntityManager& entity_manager);
export void do_ap(Ship& ship, EntityManager& entity_manager);

/// \brief Recharges fuel, destruct ordnance, and resources for divine/deity
/// ships.
export void do_god(Ship& ship, EntityManager& entity_manager);

/// \brief Processes supernova radiation and blast wave damage on ships in the
/// system.
/// \param ship Ship in the star system.
/// \param star Star undergoing supernova.
/// \param state Server state containing segment count.
/// \param em Entity manager for destroying destroyed ships.
/// \return True if ship survived, false if destroyed.
export bool process_ship_supernova(Ship& ship, const Star& star,
                                   const ServerState& state, EntityManager& em);

/// \brief Synchronizes docked ship ownership with its carrier ship.
/// \param ship Docked ship to synchronize.
/// \param em Entity manager for querying the carrier ship.
export void synchronize_docked_carrier_ownership(Ship& ship, EntityManager& em);

/// \brief Updates star and planet exploration/inhabitation status for a ship.
/// \param ship The ship being processed.
/// \param em Entity manager for mutating star and planet exploration flags.
export void update_ship_inhabited_and_exploration(const Ship& ship,
                                                  EntityManager& em);

/// \brief Accumulates ship counts, population, fuel, resources, and ordnance
/// into turn statistics for power ratings and census reporting.
/// \param ship Ship to tally.
/// \param stats Turn statistics accumulator.
/// \param update Whether this is a full turn update pass (true) or segment
/// (false).
export void accumulate_ship_power_stats(const Ship& ship, TurnStats& stats,
                                        bool update);

/// \brief Evaluates environmental hazards (such as supernovae) against an
/// active ship.
/// \param ship Ship undergoing hazard evaluation.
/// \param entity_manager Entity manager for star and server state queries.
/// \return True if ship survived all hazards, false if destroyed.
export bool evaluate_ship_hazards(Ship& ship, EntityManager& entity_manager);

/// \brief Dispatches ship subsystem simulation routines (bombardment, repairs,
/// and specialized ship role handlers).
/// \param ship Active ship.
/// \param update Whether this is a full turn update pass (true) or segment
/// (false).
/// \param entity_manager Entity manager for mutations.
/// \param stats Turn stats accumulator.
export void dispatch_ship_subsystems(Ship& ship, bool update,
                                     EntityManager& entity_manager,
                                     TurnStats& stats);
