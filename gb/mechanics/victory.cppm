// SPDX-License-Identifier: Apache-2.0

/// \file victory.cppm
/// \brief Victory condition ranking calculation across all races.

export module gb.mechanics:victory;

import gb.entities;
import gb.services;
import std;

/// \brief Builds a sorted list of player victory standings from current race
/// statistics.
export [[nodiscard]] std::vector<Victory>
create_victory_list(EntityManager& entity_manager);

/// \brief Recomputes aggregated alliance power block statistics across all
/// races and power reports.
export void compute_power_blocks(EntityManager& entity_manager);
