// SPDX-License-Identifier: Apache-2.0

/// \file session_registry.cppm
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

export module gb.services:sessionregistry;

import gb.entities;
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

/// Notification staged in the transactional socket outbox awaiting SQLite
/// commit.
export struct StagedNotification {
  player_t player;
  governor_t governor;
  std::string message;
  bool is_broadcast{false};
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

  // --- Transactional Socket Outbox ---

  /// Begin staging live socket notifications in the transactional outbox.
  virtual void begin_outbox() {
    outbox_.clear();
    outbox_active_ = true;
  }

  /// Flush all staged notifications through notify_race/notify_player and
  /// return any player-targeted notifications that failed delivery during
  /// flush.
  virtual std::vector<StagedNotification> commit_outbox() {
    outbox_active_ = false;
    auto staged = std::exchange(outbox_, {});
    std::vector<StagedNotification> undelivered;
    for (auto& item : staged) {
      if (item.is_broadcast) {
        notify_race(item.player, item.message);
      } else if (!notify_player(item.player, item.governor, item.message)) {
        undelivered.push_back(std::move(item));
      }
    }
    return undelivered;
  }

  /// Discard all staged notifications without delivering them.
  virtual void rollback_outbox() {
    outbox_.clear();
    outbox_active_ = false;
  }

  /// Check whether the transactional socket outbox is currently active.
  [[nodiscard]] bool outbox_active() const noexcept {
    return outbox_active_;
  }

  /// Check if a player/governor can receive a real-time notification.
  [[nodiscard]] virtual bool is_player_connected(player_t race,
                                                 governor_t gov) const {
    return is_connected(race, gov);
  }

  // --- Update state ---

  /// Check if updates are in progress (suppress real-time notifications)
  [[nodiscard]] virtual bool update_in_progress() const = 0;

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
  [[nodiscard]] virtual bool is_connected(player_t, governor_t) const {
    return false;  // Default: nobody is connected
  }

  /// Get list of connected sessions (for 'who' command)
  /// Returns vector of SessionInfo for all connected players
  [[nodiscard]] virtual std::vector<SessionInfo>
  get_connected_sessions() const {
    return {};  // Default: no sessions in test mode
  }

  // --- Turn scheduling requests ---

  /// Request that the server execute the next turn simulation step (segment or
  /// update)
  virtual void request_next_thing() {
    // Default implementation does nothing
  }

  /// Check if a turn simulation step is requested
  [[nodiscard]] virtual bool has_pending_turn() const {
    return false;
  }

  /// Clear pending turn request flag
  virtual void clear_pending_turn() {
    // Default implementation does nothing
  }

protected:
  bool stage_race_notification(player_t race, const std::string& message) {
    if (!outbox_active_) return false;
    outbox_.push_back({
        .player = race,
        .governor = 0,
        .message = message,
        .is_broadcast = true,
    });
    return true;
  }

  bool stage_player_notification(player_t race, governor_t gov,
                                 const std::string& message) {
    if (!outbox_active_) return false;
    outbox_.push_back({
        .player = race,
        .governor = gov,
        .message = message,
        .is_broadcast = false,
    });
    return true;
  }

private:
  bool outbox_active_{false};
  std::vector<StagedNotification> outbox_;
};

/// Null implementation of SessionRegistry for tests (does nothing)
export class NullSessionRegistry : public SessionRegistry {
  bool pending_turn_{false};

public:
  void notify_race(player_t, const std::string&) override {
    // No sessions in test mode - silently ignore
  }

  bool notify_player(player_t, governor_t, const std::string&) override {
    return false;  // Not delivered in test mode
  }

  [[nodiscard]] bool update_in_progress() const override {
    return false;  // Never in update mode during tests
  }

  void request_next_thing() override {
    pending_turn_ = true;
  }

  [[nodiscard]] bool has_pending_turn() const override {
    return pending_turn_;
  }

  void clear_pending_turn() override {
    pending_turn_ = false;
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
