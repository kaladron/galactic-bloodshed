// SPDX-License-Identifier: Apache-2.0

/// \file gblib.cppm
/// \brief Legacy monolithic module interface aggregating subsystem partitions.

export module gblib;

export import strong_id;  // Third-party strong type ID system
export import gb.entities;
export import gb.services;

export import :bombard;
export import :build;
export import :doplanet;
export import :dosector;
export import :doship;
export import :doturncmd;
export import :fire;
export import :fuel;
export import :map;
export import :misc;
export import :move;
export import :order;
export import :shlmisc;
export import :shootblast;
export import :turnstats;

export std::vector<Victory> create_victory_list(EntityManager&);
