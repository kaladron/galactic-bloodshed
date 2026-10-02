// SPDX-License-Identifier: Apache-2.0

/// \file session.cppm
/// \brief Client session management for the game server
///
/// Part of the Service Layer. Provides:
/// - Session: Individual client connection with async I/O
/// - SessionRegistry interface is in gblib (cross-cutting concern)

export module session;

import gb.entities;
import gb.services;
import asio;
import std;

// SessionRegistry interface and helpers are in gblib as cross-cutting concerns
// Commands should import gblib to access them

/// Represents a single client connection
export class Session final : public std::enable_shared_from_this<Session> {
  struct PrivateToken {
    explicit PrivateToken() = default;
  };

public:
  using DisconnectHandler = std::function<void(std::shared_ptr<Session>)>;

  [[nodiscard]] static std::shared_ptr<Session>
  create(asio::ip::tcp::socket socket, EntityManager& em,
         SessionRegistry& registry, DisconnectHandler on_disconnect = nullptr);

  Session(PrivateToken, asio::ip::tcp::socket socket, EntityManager& em,
          SessionRegistry& registry, DisconnectHandler on_disconnect);
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
    return game_obj_.player();
  }
  governor_t governor() const {
    return game_obj_.governor();
  }
  bool god() const {
    return game_obj_.god();
  }
  starnum_t snum() const {
    return game_obj_.snum();
  }
  planetnum_t pnum() const {
    return game_obj_.pnum();
  }
  shipnum_t shipno() const {
    return game_obj_.shipno();
  }
  ScopeLevel level() const {
    return game_obj_.level();
  }

  void set_player(player_t p) {
    game_obj_.set_player(p);
  }
  void set_governor(governor_t g) {
    game_obj_.set_governor(g);
  }
  void set_god(bool g) {
    game_obj_.set_god(g);
  }
  void set_snum(starnum_t s) {
    game_obj_.set_snum(s);
  }
  void set_pnum(planetnum_t p) {
    game_obj_.set_pnum(p);
  }
  void set_shipno(shipnum_t s) {
    game_obj_.set_shipno(s);
  }
  void set_level(ScopeLevel l) {
    game_obj_.set_level(l);
  }

  /// Access the persistent command execution context owned by this session
  [[nodiscard]] GameObj& game_obj() noexcept {
    return game_obj_;
  }
  [[nodiscard]] const GameObj& game_obj() const noexcept {
    return game_obj_;
  }

  // Access EntityManager for commands
  EntityManager& entity_manager() {
    return game_obj_.entity_manager;
  }

  // Access SessionRegistry for cross-player notifications
  SessionRegistry& registry() {
    return game_obj_.session_registry;
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
  asio::streambuf input_buffer_{MAX_COMMAND_LEN * 16};
  std::ostringstream out_buffer_;        // Where out() writes go
  std::deque<std::string> write_queue_;  // Pending async writes (internal)
  std::deque<std::string> input_queue_;

  GameObj game_obj_;  // Single source of truth for player, scope, and viewport
  bool connected_ = false;
  bool writing_ = false;
  int quota_ = COMMAND_BURST_SIZE;
  std::time_t last_time_ = 0;

  DisconnectHandler on_disconnect_;
};
