// SPDX-License-Identifier: Apache-2.0

/// \file server.cc
/// \brief Server service implementation.

module;

#include <csignal>
#include <cstdio>

module server;

import asio;
import auth;
import commands;
import dallib;
import gb.entities;
import gb.services;
import notification;
import session;
import std;

Server::Server(asio::io_context& io, int port, EntityManager& em)
    : io_(io),
      acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v6(), port)),
      timer_(io), signals_(io, SIGINT, SIGTERM), entity_manager_(em),
      last_quota_update_(std::chrono::steady_clock::now()) {
  // Set socket options (equivalent to old setsockopt calls)
  acceptor_.set_option(asio::socket_base::reuse_address(true));
  acceptor_.set_option(asio::socket_base::keep_alive(true));

  // Handle signals for graceful shutdown
  signals_.async_wait([this](asio::error_code ec, int signum) {
    if (!ec) {
      std::println(stderr, "Received signal {}, shutting down...", signum);
      shutdown();
    }
  });
}

void Server::start() {
  if (started_) return;
  started_ = true;
  do_accept();
  schedule_next_event();
}

void Server::run() {
  start();
  io_.run();
}

void Server::shutdown() {
  shutdown_flag_ = true;
  signals_.cancel();
  timer_.cancel();
  acceptor_.close();
  // Copy sessions to avoid iterator invalidation during disconnect
  std::vector<std::shared_ptr<Session>> all_sessions(sessions_.begin(),
                                                     sessions_.end());
  for (auto& session : all_sessions) {
    session->disconnect();
  }
}

void Server::do_accept() {
  acceptor_.async_accept(
      [this](asio::error_code ec, asio::ip::tcp::socket socket) {
        if (ec) {
          if (!shutdown_flag_) {
            std::println(stderr, "Accept error: {}", ec.message());
          }
          return;
        }

        auto session = std::make_shared<Session>(
            std::move(socket), entity_manager_, *this,
            [this](std::shared_ptr<Session> s) { remove_session(s); });
        sessions_.insert(session);
        welcome_user(*session, entity_manager_);
        session->start();

        do_accept();  // Accept next connection
      });
}

void Server::schedule_next_event() {
  timer_.expires_after(std::chrono::milliseconds(100));  // 100ms tick
  timer_.async_wait([this](asio::error_code ec) {
    if (ec || shutdown_flag_) return;
    on_timer();
    schedule_next_event();
  });
}

void Server::on_timer() {
  // Update quotas (rate limiting for commands)
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - last_quota_update_);
  if (elapsed.count() >= COMMAND_TIME_MSEC) {
    int nslices = elapsed.count() / COMMAND_TIME_MSEC;
    for (auto& session : sessions_) {
      session->add_quota(COMMANDS_PER_TIME * nslices);
    }
    last_quota_update_ = now;
  }

  // Process pending commands from all sessions
  process_commands();

  // Check for idle sessions (disconnect after IDLE_TIMEOUT_SECONDS)
  check_idle_sessions();

  // Time-based game events (updates/segments)
  std::time_t current_time = std::time(nullptr);
  const auto* state = entity_manager_.peek_server_state();
  if (state && go_time_ == 0) {
    if (current_time >= state->next_update_time) {
      go_time_ = current_time +
                 (int_rand(0, DEFAULT_RANDOM_UPDATE_RANGE.count()) * 60);
    } else if (current_time >= state->next_segment_time &&
               state->nsegments_done < state->segments) {
      go_time_ = current_time +
                 (int_rand(0, DEFAULT_RANDOM_SEGMENT_RANGE.count()) * 60);
    }
  }
  if (pending_turn_ || (go_time_ > 0 && current_time >= go_time_)) {
    do_next_thing(entity_manager_, *this);
    go_time_ = 0;
    pending_turn_ = false;
  }
}

void Server::check_idle_sessions() {
  std::time_t now = std::time(nullptr);
  std::vector<std::shared_ptr<Session>> to_disconnect;

  for (auto& session : sessions_) {
    if (session->connected() &&
        (now - session->last_time()) > IDLE_TIMEOUT_SECONDS) {
      std::println(stderr, "Disconnecting idle session (timeout)");
      session->out() << "Connection timed out due to inactivity.\n";
      to_disconnect.push_back(session);
    }
  }

  // Disconnect after iteration to avoid iterator invalidation
  for (auto& session : to_disconnect) {
    session->disconnect();
  }
}

void Server::process_commands() {
  std::vector<std::shared_ptr<Session>> to_disconnect;

  // Execute pending commands for all sessions
  for (auto& session : sessions_) {
    while (session->quota() > 0 && session->has_pending_input()) {
      std::string command = session->pop_input();
      session->use_quota();
      session->touch();

      if (!do_command(*session, command)) {
        to_disconnect.push_back(session);
        break;
      }
    }
  }

  // Disconnect sessions that returned false (quit command, etc.)
  // Do this before flushing to avoid sending output to disconnected sessions
  for (auto& session : to_disconnect) {
    session->disconnect();
  }

  // Check if shutdown was requested during command processing
  if (shutdown_flag_) {
    shutdown();
    return;  // Don't flush output, server is shutting down
  }

  // Flush all dirty output buffers to network
  // This handles both direct command output AND cross-player notifications
  for (auto& session : sessions_) {
    session->flush_to_network();
  }
}

void Server::notify_race(player_t race, const std::string& message) {
  if (update_in_progress()) return;
  for (auto& session : sessions_) {
    if (session->connected() && session->player() == race) {
      session->out() << message;
    }
  }
}

bool Server::notify_player(player_t race, governor_t gov,
                           const std::string& message) {
  if (update_in_progress()) return false;
  bool delivered = false;
  for (auto& session : sessions_) {
    if (session->connected() && session->player() == race &&
        session->governor() == gov) {
      session->out() << message;
      delivered = true;
    }
  }
  return delivered;
}

void Server::flush_all() {
  for (auto& session : sessions_) {
    session->flush_to_network();
  }
}

bool Server::is_connected(player_t race, governor_t gov) const {
  for (const auto& session : sessions_) {
    if (session->connected() && session->player() == race &&
        session->governor() == gov) {
      return true;
    }
  }
  return false;
}

std::vector<SessionInfo> Server::get_connected_sessions() const {
  std::vector<SessionInfo> result;
  for (const auto& session : sessions_) {
    if (session->connected()) {
      result.push_back({.player = session->player(),
                        .governor = session->governor(),
                        .snum = session->snum(),
                        .connected = true,
                        .god = session->god(),
                        .last_time = session->last_time()});
    }
  }
  return result;
}

void Server::remove_session(std::shared_ptr<Session> session) {
  if (session->connected()) {
    std::println(stderr, "DISCONNECT Race={} Governor={}", session->player(),
                 session->governor());
  } else {
    std::println(stderr, "DISCONNECT never connected");
  }
  sessions_.erase(session);
}

bool Server::do_command(Session& session, std::string_view comm) {
  if (session.connected()) {
    auto argv = make_command_t(comm);
    GameObj g(session.entity_manager(), session.registry());
    g.set_player(session.player());
    g.set_governor(session.governor());
    g.set_god(session.god());
    g.set_snum(session.snum());
    g.set_pnum(session.pnum());
    g.set_shipno(session.shipno());
    g.set_level(session.level());
    g.race = session.entity_manager().peek_race(g.player());

    process_command(g, argv);

    if (g.shutdown_requested()) {
      shutdown_flag_ = true;
    }

    if (g.disconnect_requested()) {
      session.out() << g.out.str();
      return false;
    }

    session.set_player(g.player());
    session.set_governor(g.governor());
    session.set_god(g.god());
    session.set_snum(g.snum());
    session.set_pnum(g.pnum());
    session.set_shipno(g.shipno());
    session.set_level(g.level());

    session.out() << g.out.str();
  } else {
    check_connect(session, comm);
    if (!session.connected()) {
      session.out() << "Goodbye!\n";
      return false;
    }
    GameObj g(session.entity_manager(), session.registry());
    g.set_player(session.player());
    g.set_governor(session.governor());
    g.set_god(session.god());
    g.set_snum(session.snum());
    g.set_pnum(session.pnum());
    g.set_shipno(session.shipno());
    g.set_level(session.level());
    g.race = session.entity_manager().peek_race(g.player());

    check_for_telegrams(g);

    command_t call_cs = {"cs"};
    process_command(g, call_cs);

    session.set_snum(g.snum());
    session.set_pnum(g.pnum());
    session.set_shipno(g.shipno());
    session.set_level(g.level());

    session.out() << g.out.str();
  }
  return true;
}

void Server::process_command(GameObj& g, const command_t& argv) {
  if (argv.empty()) return;

  const auto* race = g.entity_manager.peek_race(g.player());
  if (!race) {
    g.out << "Error: Could not find your race.\n";
    return;
  }
  g.race = race;

  if (const auto* desc = GB::commands::find_command_descriptor(argv[0])) {
    GB::commands::dispatch_command(g, *desc, argv);
  } else {
    g.out << "'" << argv[0] << "':illegal command error.\n";
  }

  /* compute the prompt and send to the player */
  g.out << do_prompt(g);
  g.race = nullptr;
}
