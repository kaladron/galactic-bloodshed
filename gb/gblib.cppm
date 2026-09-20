// SPDX-License-Identifier: Apache-2.0

/// \file gblib.cppm
/// \brief Legacy monolithic module interface aggregating subsystem partitions.

export module gblib;

export import strong_id;  // Third-party strong type ID system
export import gb.entities;
export import gb.services;
export import gb.mechanics;

export import :bombard;
export import :build;
export import :doplanet;
export import :dosector;
export import :doship;
export import :doturncmd;
export import :fire;
export import :map;
export import :misc;
export import :move;
export import :shlmisc;
export import :shootblast;
export import :turnstats;

export std::vector<Victory> create_victory_list(EntityManager&);
