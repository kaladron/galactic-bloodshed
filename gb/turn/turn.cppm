// SPDX-License-Identifier: Apache-2.0

/// \file turn.cppm
/// \brief Subsystem module exporting turn engine processing passes and
/// commands.

module;

export module gb.turn;

import gb.entities;
import gb.services;
import gb.mechanics;

export import :turnstats;
export import :bombard;
export import :doplanet;
export import :dosector;
export import :doship;
export import :vn;
export import :doturncmd;
