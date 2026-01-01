// SPDX-License-Identifier: Apache-2.0

/// \file notification.cppm
/// \brief Notification service for sending messages to connected clients
///
/// Part of the Service Layer. Provides message routing to clients based on
/// race, governor, star system, and gag settings.

export module notification;

import session; // SessionRegistry is in standalone session module
import gblib;   // For types, EntityManager
import std;

// Note: EntityManager is already available via gblib import

// Note: notify_race() and notify_player() are now methods on SessionRegistry
// (see session.cppm). They only need session iteration, not game logic.
// The functions below have game logic (gag checks, telegram fallback, etc.)

/// Broadcast message to all connected clients (except sender), respects gag
export void d_broadcast(SessionRegistry& registry, EntityManager& em,
                        player_t sender, governor_t sender_gov,
                        const std::string& message);

/// Announce message to players in a star system, respects gag
export void d_announce(SessionRegistry& registry, EntityManager& em,
                       player_t sender, governor_t sender_gov, starnum_t star,
                       const std::string& message);

/// Send think message to other governors of same race
export void d_think(SessionRegistry& registry, EntityManager& em, player_t race,
                    governor_t sender_gov, const std::string& message);

/// Shout message to all clients (ignores gag)
export void d_shout(SessionRegistry& registry, player_t sender,
                    governor_t sender_gov, const std::string& message);

/// Warn a specific player's governor, falls back to governor 0, then telegram
export void warn_player(SessionRegistry& registry, player_t who, governor_t gov,
                        const std::string& message);

/// Warn all governors of a race
export void warn_race(SessionRegistry& registry, EntityManager& em,
                      player_t who, const std::string& message);
