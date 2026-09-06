// SPDX-License-Identifier: Apache-2.0

/// \file gblib-bombard.cppm
/// \brief Module interface partition for autonomous berserker planetary
/// bombardment.

export module gblib:bombard;

import :planet;
import :race;
import :services;
import :ships;
import :shootblast;
import :star;
import :turnstats;
import :types;
import std;

/// \brief Checks whether any active foreign Point Defense Networks (PDNs) are
/// in orbit protecting the planet.
/// \param entity_manager Entity manager for spatial ship queries.
/// \param planet Target planet orbited.
/// \param attacker Attacking player ID (friendly PDNs do not block
/// bombardment).
/// \return True if hostile/foreign PDNs prevent bombardment, false otherwise.
export bool check_orbital_pdn_defense(EntityManager& entity_manager,
                                      const Planet& planet, player_t attacker);

/// \brief Identifies a suitable planetary sector to bombard.
///
/// Priority order:
/// 1. Hostile colony owned by a race at war with attacker or programmed target.
/// 2. Any foreign colony bombardable by the attacker.
///
/// \param entity_manager Entity manager for sectormap access.
/// \param ship Bombarding ship (used for owner, target, and location).
/// \param attacker_race Attacking player's race for diplomatic state checks.
/// \return Target coordinates if a candidate sector exists, nullopt otherwise.
export std::optional<Coordinates>
find_bombardment_target(EntityManager& entity_manager, const Ship& ship,
                        const Race& attacker_race);

/// \brief Calculates the effective bombardment strength based on guns, hull
/// efficiency, and available destruct crystals.
/// \param ship Bombarding ship.
/// \return Weapon strength capped by available destruct resources.
export int calculate_bombardment_strength(const Ship& ship);

/// \brief Dispatches telegram reports and alerts to the attacker and victim
/// races, and posts public combat news.
/// \param entity_manager Entity manager for messaging.
/// \param ship Bombarding ship.
/// \param star Star system orbited.
/// \param target Targeted planetary sector coordinates.
/// \param old_owner Player owning the targeted sector before bombardment.
/// \param sectors_destroyed Number of sectors destroyed.
/// \param result Combat bombardment resolution details.
export void dispatch_bombardment_alerts(EntityManager& entity_manager,
                                        const Ship& ship, const Star& star,
                                        Coordinates target, player_t old_owner,
                                        int sectors_destroyed,
                                        const BombardResult& result);

/// \brief Simulates autonomous berserker orbital bombardment against enemy
/// colonies on planetary surfaces.
/// \param entity_manager Entity manager for spatial queries and mutations.
/// \param ship Berserker ship executing bombardment.
/// \param planet Target planet in orbit.
/// \param r Attacking race for alliance and diplomatic state checks.
/// \return Count of planetary sectors destroyed.
export int berserker_bombard(EntityManager& entity_manager, Ship& ship,
                             Planet& planet, const Race& r);
