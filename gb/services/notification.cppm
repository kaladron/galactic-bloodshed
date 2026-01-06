// SPDX-License-Identifier: Apache-2.0

/// \file notification.cppm
/// \brief Notification service for sending messages to connected clients
///
/// Part of the Service Layer. Provides message routing to clients based on
/// race, governor, star system, and gag settings.

export module notification;

import gblib; // For SessionRegistry, types, EntityManager
import std;

// Note: SessionRegistry and its basic notify methods are in gblib
// (cross-cutting). The functions below add game logic (gag checks, telegram
// fallback, star-based routing).

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
export void d_shout(SessionRegistry& registry, EntityManager& em,
                    player_t sender, governor_t sender_gov,
                    const std::string& message);

/// Warn a specific player's governor, falls back to governor 0, then telegram
export void warn_player(SessionRegistry& registry, player_t who, governor_t gov,
                        const std::string& message);

/// Warn all governors of a race
export void warn_race(SessionRegistry& registry, EntityManager& em,
                      player_t who, const std::string& message);

/// Send message to all players who inhabit a star system
export void notify_star(SessionRegistry& registry, EntityManager& em,
                        player_t sender, governor_t sender_gov, starnum_t star,
                        const std::string& message);

/// Send message to all governors of all players who inhabit a star system
export void warn_star(SessionRegistry& registry, EntityManager& em,
                      player_t sender, starnum_t star,
                      const std::string& message);
