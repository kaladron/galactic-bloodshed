// SPDX-License-Identifier: Apache-2.0

/// \file gblib-doship.cppm
/// \brief Module interface partition for ship turn simulation processing.

export module gblib:doship;

import :gameobj;
import :ships;
import :star;
import :turnstats;

export void doship(Ship&, bool update, EntityManager&, TurnStats& stats);
export void domass(Ship&, EntityManager&);
export void doown(Ship&, EntityManager&);
export void domissile(Ship&, EntityManager&);
/// \brief Simulates proximity triggering, detonation, ship collateral damage,
/// and orbital planetary bombardment for space mines.
/// \param ship Mine ship executing turn processing.
/// \param detonate Whether manual or forced detonation is triggered (non-zero).
/// \param entity_manager Entity manager for spatial queries and mutations.
export void domine(Ship& ship, int detonate, EntityManager& entity_manager);

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
export void do_pod(Ship& ship, EntityManager& entity_manager);

/// \brief Simulates dust canister atmospheric cooling and eventual dissipation.
export void do_canister(Ship& ship, EntityManager& entity_manager,
                        TurnStats& stats);

/// \brief Simulates greenhouse gas warming and eventual dissipation.
export void do_greenhouse(Ship& ship, EntityManager& entity_manager,
                          TurnStats& stats);

/// \brief Simulates focused space mirror heating against ships, planets, or
/// stars.
export void do_mirror(Ship& ship, EntityManager& entity_manager,
                      TurnStats& stats);
export void do_meta_infect(player_t who, starnum_t star, planetnum_t pnum,
                           Planet& p, EntityManager& entity_manager);
export int infect_planet(player_t who, starnum_t star, planetnum_t pnum,
                         EntityManager& entity_manager);
export void do_ap(Ship& ship, EntityManager& entity_manager);

/// \brief Recharges fuel, destruct ordnance, and resources for divine/deity
/// ships.
export void do_god(Ship& ship, EntityManager& entity_manager);

/// \brief Processes radiation effects on ship crew and accumulated radiation
/// decay.
/// \param ship Ship experiencing radiation.
/// \param update Whether this is a full turn update pass (true) or segment
/// (false).
/// \return Whether the ship remains active/mobile after radiation checks.
export bool process_ship_radiation(Ship& ship, bool update);

/// \brief Processes supernova radiation and blast wave damage on ships in the
/// system.
/// \param ship Ship in the star system.
/// \param star Star undergoing supernova.
/// \param state Server state containing segment count.
/// \param em Entity manager for destroying destroyed ships.
/// \return True if ship survived, false if destroyed.
export bool process_ship_supernova(Ship& ship, const Star& star,
                                   const ServerState& state, EntityManager& em);

/// \brief Synchronizes offline factory technological capability with current
/// empire technology.
/// \param ship Factory ship to synchronize.
/// \param race Race owning the factory.
export void sync_factory_technology(Ship& ship, const Race& race);

/// \brief Synchronizes docked ship ownership with its carrier ship.
/// \param ship Docked ship to synchronize.
/// \param em Entity manager for querying the carrier ship.
export void synchronize_docked_carrier_ownership(Ship& ship, EntityManager& em);

/// \brief Updates star and planet exploration/inhabitation status for a ship.
/// \param ship Ship exploring or inhabiting the system.
/// \param em Entity manager for mutating star and planet exploration flags.
/// \param stats Per-turn stats tracking inhabited stars.
export void update_ship_inhabited_and_exploration(const Ship& ship,
                                                  EntityManager& em,
                                                  TurnStats& stats);

/// \brief Accumulates ship counts, population, fuel, resources, and ordnance
/// into turn statistics for power ratings and census reporting.
/// \param ship Ship to tally.
/// \param stats Turn statistics accumulator.
/// \param update Whether this is a full turn update pass (true) or segment
/// (false).
export void accumulate_ship_power_stats(const Ship& ship, TurnStats& stats,
                                        bool update);

/// \brief Top two nearest star systems identified by navigation scanning.
export struct StarTargetResult {
  starnum_t closest{0};         ///< Primary nearest star system
  starnum_t second_closest{0};  ///< Secondary nearest star system
};

/// \brief Finds the closest and second-closest star systems to the given
/// coordinates, excluding the current star system.
export StarTargetResult find_closest_stars(EntityManager& em,
                                           starnum_t current_star, double xpos,
                                           double ypos);

/// \brief Assigns destination orders to an autonomous berserker ship.
export void select_berserker_destination(EntityManager& em,
                                         AutonomousShip& ship,
                                         const TurnStats& stats);

/// \brief Assigns destination orders to an autonomous Von Neumann machine.
export void select_vn_destination(EntityManager& em, AutonomousShip& ship);

/// \brief Result of stealing planetary resources from an alien colony.
export struct StealResult {
  player_t victim{0};    ///< Player ID victimized, or 0 if none
  resource_t amount{0};  ///< Quantity of resources stolen
};

/// \brief Steals resources from alien colonies on the currently landed planet.
export StealResult steal_planetary_resources(EntityManager& em,
                                             AutonomousShip& ship);

/// \brief Mines resources from a sector, transferring extracted yield to cargo
/// and fuel.
export resource_t mine_sector(AutonomousShip& ship, Sector& sector);

/// \brief Moves an autonomous machine to an adjacent sector when current sector
/// is depleted.
export Coordinates roam_to_adjacent_sector(AutonomousShip& ship,
                                           const Planet& planet);

/// \brief Generates a random binary name for a new Von Neumann machine.
export std::string generate_vn_binary_name();

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
