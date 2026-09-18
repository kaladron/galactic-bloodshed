// SPDX-License-Identifier: Apache-2.0

/// \file server.cppm
/// \brief Server service module implementing SessionRegistry and asynchronous
/// network loop.

module;

#include <csignal>

export module server;

import asio;
import auth;
import commands;
import dallib;
import gb.entities;
import gb.services;
import notification;
import session;
import std;

export class Server : public SessionRegistry {
public:
  Server(asio::io_context& io, int port, EntityManager& em);

  void start();
  void run();
  void shutdown();

  // SessionRegistry interface - notification primitives
  void notify_race(player_t race, const std::string& message) override;
  bool notify_player(player_t race, governor_t gov,
                     const std::string& message) override;
  bool update_in_progress() const override {
    return update_flag_;
  }
  void set_update_in_progress(bool v) override {
    update_flag_ = v;
  }
  void flush_all() override;
  bool is_connected(player_t race, governor_t gov) const override;
  std::vector<SessionInfo> get_connected_sessions() const override;
  void request_next_thing() override {
    pending_turn_ = true;
  }
  [[nodiscard]] bool has_pending_turn() const override {
    return pending_turn_;
  }
  void clear_pending_turn() override {
    pending_turn_ = false;
  }

  EntityManager& entity_manager() {
    return entity_manager_;
  }

  unsigned short port() const {
    return acceptor_.local_endpoint().port();
  }

  std::size_t session_count() const {
    return sessions_.size();
  }

  void on_timer();
  void update_quotas(std::chrono::steady_clock::time_point now =
                         std::chrono::steady_clock::now());
  void process_commands();
  void check_idle_sessions(std::time_t now = std::time(nullptr));
  void check_turn_events(std::time_t current_time = std::time(nullptr));

private:
  void do_accept();
  void schedule_next_event();
  void remove_session(std::shared_ptr<Session> session);
  bool do_command(Session& session, std::string_view comm);
  void process_command(GameObj& g, const command_t& argv);

  asio::io_context& io_;
  asio::ip::tcp::acceptor acceptor_;
  asio::steady_timer timer_;
  asio::signal_set signals_;  // For graceful shutdown on SIGINT/SIGTERM
  EntityManager& entity_manager_;

  std::set<std::shared_ptr<Session>> sessions_;
  bool started_ = false;
  bool shutdown_flag_ = false;
  bool update_flag_ = false;
  bool pending_turn_ = false;

  std::time_t go_time_ = 0;
  std::chrono::steady_clock::time_point last_quota_update_;
};
