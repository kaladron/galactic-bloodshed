// SPDX-License-Identifier: Apache-2.0

/// \file gblib.cppm
/// \brief Legacy monolithic module interface aggregating subsystem partitions.

export module gblib;

export import strong_id;  // Third-party strong type ID system
export import gb.entities;
export import gb.services;
export import gb.mechanics;

export import :bombard;
export import :doplanet;
export import :dosector;
export import :doship;
export import :doturncmd;
export import :turnstats;
