// SPDX-License-Identifier: Apache-2.0

export module gblib:bombard;

import :planet;
import :race;
import :services;
import :ships;
import :turnstats;

/// \brief Simulates autonomous berserker orbital bombardment against enemy
/// colonies on planetary surfaces.
/// \param entity_manager Entity manager for spatial queries and mutations.
/// \param ship Berserker ship executing bombardment.
/// \param planet Target planet in orbit.
/// \param r Attacking race for alliance and diplomatic state checks.
/// \return Count of planetary sectors destroyed.
export int berserker_bombard(EntityManager& entity_manager, Ship& ship,
                             Planet& planet, const Race& r);
