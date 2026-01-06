// SPDX-License-Identifier: Apache-2.0

/// \file gblib-sessionregistry.cppm
/// \brief SessionRegistry interface - cross-cutting concern for session
/// management
///
/// This is a cross-cutting interface that provides NOTIFICATION PRIMITIVES.
/// Commands and services use these primitives to send messages to connected
/// players. The actual implementation (Server class) is in the application
/// layer (GB_server.cc).
///
/// Complex notification logic (gag checks, star system filtering) belongs in
/// the notification service layer, which uses these primitives.

export module gblib:sessionregistry;

import :types;
import std;

/// Session metadata for the 'who' command (without exposing Session type)
export struct SessionInfo {
  player_t player;
  governor_t governor;
  starnum_t snum;
  bool connected;
  bool god;
  std::time_t last_time;
};

/// Abstract interface for session management (cross-cutting concern)
/// Provides notification primitives that don't require game state knowledge.
/// Implementations are in the application layer (Server class).
export class SessionRegistry {
public:
  virtual ~SessionRegistry() = default;

  // Rule of 5 - make non-copyable, non-movable
  SessionRegistry() = default;
  SessionRegistry(const SessionRegistry&) = delete;
  SessionRegistry& operator=(const SessionRegistry&) = delete;
  SessionRegistry(SessionRegistry&&) = delete;
  SessionRegistry& operator=(SessionRegistry&&) = delete;

  // --- Notification primitives ---

  /// Send message to all governors of a race who are currently connected
  virtual void notify_race(player_t race, const std::string& message) = 0;

  /// Send message to a specific player's governor if connected
  /// Returns true if message was delivered to at least one session
  virtual bool notify_player(player_t race, governor_t gov,
                             const std::string& message) = 0;

  // --- Update state ---

  /// Check if updates are in progress (suppress real-time notifications)
  virtual bool update_in_progress() const = 0;

  /// Set update in progress flag (used by turn processing)
  virtual void set_update_in_progress(bool) {
    // Default implementation does nothing
  }

  // --- Session management ---

  /// Flush all session output buffers to network (for immediate delivery)
  virtual void flush_all() {
    // Default implementation does nothing
  }

  /// Check if a player/governor is currently connected
  virtual bool is_connected(player_t, governor_t) const {
    return false;  // Default: nobody is connected
  }

  /// Get list of connected sessions (for 'who' command)
  /// Returns vector of SessionInfo for all connected players
  virtual std::vector<SessionInfo> get_connected_sessions() const {
    return {};  // Default: no sessions in test mode
  }
};

/// Null implementation of SessionRegistry for tests (does nothing)
export class NullSessionRegistry : public SessionRegistry {
public:
  void notify_race(player_t, const std::string&) override {
    // No sessions in test mode - silently ignore
  }

  bool notify_player(player_t, governor_t, const std::string&) override {
    return false;  // Not delivered in test mode
  }

  bool update_in_progress() const override {
    return false;  // Never in update mode during tests
  }

  NullSessionRegistry() = default;
};

/// Get singleton NullSessionRegistry instance for tests
export inline SessionRegistry& get_null_session_registry() {
  static NullSessionRegistry null_registry;
  return null_registry;
}

/// Get default SessionRegistry for GameObj (used when not explicitly set)
export inline SessionRegistry& get_default_session_registry() {
  return get_null_session_registry();
}
