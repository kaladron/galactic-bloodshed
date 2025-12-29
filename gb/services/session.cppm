// SPDX-License-Identifier: Apache-2.0

/// \file session.cppm
/// \brief Client session management for the game server
///
/// Part of the Service Layer. Provides:
/// - SessionRegistry: Abstract interface for iterating over sessions
/// - Session: Individual client connection with async I/O

export module session;

import gblib;
import asio;
import std;

export class Session;

/// Abstract interface for session management (enables testing without sockets)
export class SessionRegistry {
public:
  virtual ~SessionRegistry() = default;

  // Rule of 5 - make non-copyable, non-movable
  SessionRegistry() = default;
  SessionRegistry(const SessionRegistry&) = delete;
  SessionRegistry& operator=(const SessionRegistry&) = delete;
  SessionRegistry(SessionRegistry&&) = delete;
  SessionRegistry& operator=(SessionRegistry&&) = delete;

  /// Iterate over all connected sessions
  virtual void for_each_session(std::function<void(Session&)> fn) = 0;

  /// Check if updates are in progress (suppress notifications during updates)
  virtual bool update_in_progress() const = 0;

  // Non-virtual notification methods (implemented using for_each_session)
  // These are on SessionRegistry because they only need session iteration,
  // not game logic like telegram fallback or race lookups.

  /// Send message to all governors of a race
  void notify_race(player_t race, const std::string& message);

  /// Send message to a specific player's governor, returns true if delivered
  bool notify_player(player_t race, governor_t gov, const std::string& message);
};

/// Represents a single client connection
export class Session : public std::enable_shared_from_this<Session> {
public:
  Session(asio::ip::tcp::socket socket, EntityManager& em,
          SessionRegistry& registry,
          std::function<void(std::shared_ptr<Session>)> on_disconnect);
  ~Session() = default;

  // Non-copyable, non-movable (prevent socket duplication)
  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;
  Session(Session&&) = delete;
  Session& operator=(Session&&) = delete;

  /// Start async read loop
  void start();

  /// The ONLY output interface - buffered output stream
  /// Commands write here; cross-player notifications write here.
  /// Buffer is flushed to network after each command batch.
  std::ostream& out() {
    return out_buffer_;
  }

  /// Check if there's pending output to flush
  bool has_pending_output() const;

  /// Get total size of write queue
  std::size_t write_queue_size() const;

  /// Flush output buffer to network (called by Server after commands)
  /// Disconnects client if write queue exceeds MAX_WRITE_QUEUE_SIZE (slow
  /// client)
  void flush_to_network();

  /// Graceful disconnect
  void disconnect();

  // Connection state
  bool connected() const {
    return connected_;
  }
  void set_connected(bool c) {
    connected_ = c;
  }

  player_t player() const {
    return player_;
  }
  governor_t governor() const {
    return governor_;
  }
  bool god() const {
    return god_;
  }
  starnum_t snum() const {
    return snum_;
  }
  planetnum_t pnum() const {
    return pnum_;
  }
  shipnum_t shipno() const {
    return shipno_;
  }
  ScopeLevel level() const {
    return level_;
  }

  void set_player(player_t p) {
    player_ = p;
  }
  void set_governor(governor_t g) {
    governor_ = g;
  }
  void set_god(bool g) {
    god_ = g;
  }
  void set_snum(starnum_t s) {
    snum_ = s;
  }
  void set_pnum(planetnum_t p) {
    pnum_ = p;
  }
  void set_shipno(shipnum_t s) {
    shipno_ = s;
  }
  void set_level(ScopeLevel l) {
    level_ = l;
  }

  // Access EntityManager for commands
  EntityManager& entity_manager() {
    return entity_manager_;
  }

  // Access SessionRegistry for cross-player notifications
  SessionRegistry& registry() {
    return registry_;
  }

  // Rate limiting
  int quota() const {
    return quota_;
  }
  void add_quota(int n) {
    quota_ = std::min(quota_ + n, COMMAND_BURST_SIZE);
  }
  void use_quota() {
    if (quota_ > 0) --quota_;
  }

  // Input queue access (for command processing)
  bool has_pending_input() const {
    return !input_queue_.empty();
  }
  std::string pop_input();

  // Last activity time
  std::time_t last_time() const {
    return last_time_;
  }
  void touch() {
    last_time_ = std::time(nullptr);
  }

private:
  void do_read();
  void do_write();
  void queue_for_write(std::string content);  // Internal: add to write queue

  asio::ip::tcp::socket socket_;
  asio::streambuf input_buffer_;
  std::ostringstream out_buffer_;        // Where out() writes go
  std::deque<std::string> write_queue_;  // Pending async writes (internal)
  std::deque<std::string> input_queue_;

  EntityManager& entity_manager_;  // For creating GameObj on demand if needed
  SessionRegistry& registry_;      // For cross-player notifications
  bool connected_ = false;
  bool writing_ = false;
  int quota_ = COMMAND_BURST_SIZE;
  std::time_t last_time_ = 0;

  // Player state (was in GameObj, now directly in Session)
  player_t player_ = 0;
  governor_t governor_ = 0;
  bool god_ = false;
  starnum_t snum_ = 0;
  planetnum_t pnum_ = 0;
  shipnum_t shipno_ = 0;
  ScopeLevel level_ = ScopeLevel::LEVEL_UNIV;

  std::function<void(std::shared_ptr<Session>)> on_disconnect_;
};
