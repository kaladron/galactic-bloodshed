// SPDX-License-Identifier: Apache-2.0

/// \file GB_server.cc
/// \brief Main game server executable.

#include <sys/stat.h>
#include <unistd.h>
#include <cctype>
#include <csignal>
#include <cstdio>
#include <cstdlib>

import std;
import asio;
import auth;
import commands;
import dallib;
import gblib;
import notification;
import session;

// Server class - implements SessionRegistry interface for the application layer
class Server : public SessionRegistry {
public:
  Server(asio::io_context& io, int port, EntityManager& em);

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

  EntityManager& entity_manager() {
    return entity_manager_;
  }

private:
  void do_accept();
  void schedule_next_event();
  void on_timer();
  void process_commands();
  void check_idle_sessions();
  void remove_session(std::shared_ptr<Session> session);
  bool do_command(Session& session, std::string_view comm);

  asio::io_context& io_;
  asio::ip::tcp::acceptor acceptor_;
  asio::steady_timer timer_;
  asio::signal_set signals_;  // For graceful shutdown on SIGINT/SIGTERM
  EntityManager& entity_manager_;

  std::set<std::shared_ptr<Session>> sessions_;
  bool shutdown_flag_ = false;
  bool update_flag_ = false;

  std::time_t go_time_ = 0;
  std::chrono::steady_clock::time_point last_quota_update_;
};

bool shutdown_flag = false;  // Used by shutdown command

static void process_command(GameObj&, const command_t& argv);
static void initialize_block_data(EntityManager&);

// ============================================================================
// Server class implementation
// ============================================================================

Server::Server(asio::io_context& io, int port, EntityManager& em)
    : io_(io),
      acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v6(), port)),
      timer_(io), signals_(io, SIGINT, SIGTERM), entity_manager_(em),
      last_quota_update_(std::chrono::steady_clock::now()) {
  // Set socket options (equivalent to old setsockopt calls)
  acceptor_.set_option(asio::socket_base::reuse_address(true));
  acceptor_.set_option(asio::socket_base::keep_alive(true));

  // Handle signals for graceful shutdown (replaces set_signals())
  signals_.async_wait([this](asio::error_code ec, int signum) {
    if (!ec) {
      std::println(stderr, "Received signal {}, shutting down...", signum);
      shutdown();
    }
  });
}

void Server::run() {
  do_accept();
  schedule_next_event();
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

  // --- Time-based game events (updates/segments) ---
  // This replaces the timing logic from shovechars()
  // do_next_thing() calls either do_segment() or do_update() based on game
  // state
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
  if (go_time_ > 0 && current_time >= go_time_) {
    do_next_thing(entity_manager_, *this);
    go_time_ = 0;
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

int main(int argc, char** argv) {
  // Create Database and EntityManager for dependency injection
  Database database{PKGSTATEDIR "gb.db"};
  EntityManager entity_manager{database};

  // Get server state handle (will auto-save on scope exit)
  auto server_state_handle = entity_manager.get_server_state();
  auto& state = *server_state_handle;

  std::println(std::cout, "      ***   Galactic Bloodshed ver {0} ***",
               GB_VERSION);
  std::println(std::cout, "");
  std::time_t clk = std::time(nullptr);
  std::print("      {0}", std::ctime(&clk));
  if (EXTERNAL_TRIGGER) {
    std::println(std::cout, "      The update  password is '%s'.",
                 UPDATE_PASSWORD);
    std::println(std::cout, "      The segment password is '%s'.",
                 SEGMENT_PASSWORD);
  }
  int port;
  std::chrono::minutes update_time;  // Local for command parsing
  switch (argc) {
    case 2:
      port = std::stoi(argv[1]);
      update_time = std::chrono::minutes(DEFAULT_UPDATE_TIME);
      state.update_time_minutes = update_time.count();
      state.segments = MOVES_PER_UPDATE;
      break;
    case 3:
      port = std::stoi(argv[1]);
      update_time = std::chrono::minutes(std::stoi(argv[2]));
      state.update_time_minutes = update_time.count();
      state.segments = MOVES_PER_UPDATE;
      break;
    case 4:
      port = std::stoi(argv[1]);
      update_time = std::chrono::minutes(std::stoi(argv[2]));
      state.update_time_minutes = update_time.count();
      state.segments = std::stoi(argv[3]);
      break;
    default:
      port = GB_PORT;
      update_time = DEFAULT_UPDATE_TIME;
      state.update_time_minutes = update_time.count();
      state.segments = MOVES_PER_UPDATE;
      break;
  }
  std::cerr << "      Port " << port << '\n';
  std::cerr << "      " << update_time << " minutes between updates" << '\n';
  std::cerr << "      " << state.segments << " segments/update" << '\n';
  set_server_start_time(clk);

  // Initialize state from database or set defaults if first run
  if (state.next_update_time == 0) {
    state.next_update_time = clk + (state.update_time_minutes * 60);
  }
  if (state.segments <= 1) {
    state.next_segment_time = clk + (144 * 3600);
  } else {
    if (state.next_segment_time == 0) {
      state.next_segment_time =
          clk + (state.update_time_minutes * 60 / state.segments);
    }
    if (state.next_segment_time < clk) {
      state.next_segment_time = state.next_update_time;
      state.nsegments_done = state.segments;
    }
  }

  // Print initial schedule status
  std::print(stderr, "Last Update {:3d} : {}", 0, std::ctime(&clk));
  std::print(stderr, "Last Segment {0:2d} : {1}", state.nsegments_done,
             std::ctime(&clk));
  srandom(getpid());
  std::print(stderr, "      Next Update {0}  : {1}", 1,
             std::ctime(&state.next_update_time));
  std::print(stderr, "      Next Segment   : {0}",
             std::ctime(&state.next_segment_time));

  // Verify universe is initialized (created by makeuniv)
  const auto* universe = entity_manager.peek_universe();
  if (!universe) {
    std::println(stderr, "\nERROR: Universe not initialized!");
    std::println(stderr, "Please run 'makeuniv' to create the game universe.");
    return 1;
  }

  // Initialize game data structures
  initialize_block_data(entity_manager);  // Ensure self-invite/self-pledge
  compute_power_blocks(entity_manager);   // Calculate alliance power stats

  // Start server using new Asio-based Server class
  asio::io_context io;
  Server server(io, port, entity_manager);
  post(entity_manager, "Server started\n", NewsType::ANNOUNCE);
  server.run();

  // Save final state before shutdown
  server_state_handle.save();

  std::println(std::cout, "Going down.");
  return 0;
}

/** Main processing loop. When command strings are sent from the client,
   they are processed here. Responses are sent back to the client via
   session.out().
   */
bool Server::do_command(Session& session, std::string_view comm) {
  /* check to see if there are a few words typed out, usually for the help
   * command */
  auto argv = make_command_t(comm);

  if (session.connected() && argv[0] == "who") {
    GB::commands::who(argv, session);
  } else if (session.connected() && session.god() && argv[0] == "emulate") {
    GB::commands::emulate(argv, session);
  } else if (session.connected() && session.god() && argv[0] == "@@update") {
    const auto* race = session.entity_manager().peek_race(session.player());
    if (!race || !race->God) {
      session.out() << "Only deity can use this command.\n";
    } else {
      session.out() << "Starting update...\n";
      session.flush_to_network();
      do_update(session.entity_manager(), session.registry(), true);
      session.out() << "Update completed.\n";
    }
  } else if (session.connected() && session.god() && argv[0] == "@@segment") {
    const auto* race = session.entity_manager().peek_race(session.player());
    if (!race || !race->God) {
      session.out() << "Only deity can use this command.\n";
    } else {
      int seg_num = 0;
      if (argv.size() > 1) {
        seg_num = std::stoi(argv[1]);
      }
      session.out() << "Starting segment movement...\n";
      session.flush_to_network();
      do_segment(session.entity_manager(), session.registry(), 1, seg_num);
      session.out() << "Segment completed.\n";
    }
  } else {
    if (session.connected()) {
      /* GB command parser - create temporary GameObj */
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

      // Check if @@shutdown command was executed
      if (g.shutdown_requested()) {
        shutdown_flag_ = true;
      }

      // Check if disconnect was requested
      if (g.disconnect_requested()) {
        session.out() << g.out.str();
        return false;
      }

      // Copy any state changes back to session
      session.set_snum(g.snum());
      session.set_pnum(g.pnum());
      session.set_shipno(g.shipno());
      session.set_level(g.level());

      // Flush GameObj output to session (GameObj.out is a stringstream)
      session.out() << g.out.str();
    } else {
      // Handle login
      check_connect(session, comm);
      if (!session.connected()) {
        session.out() << "Goodbye!\n";
        return false;
      }
      // Login successful - check for telegrams and set home scope
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

      // Copy scope back
      session.set_snum(g.snum());
      session.set_pnum(g.pnum());
      session.set_shipno(g.shipno());
      session.set_level(g.level());

      // Flush GameObj output to session
      session.out() << g.out.str();
    }
  }
  return true;
}

/**
 * @brief Process a command in the game.
 *
 * This function processes a command in the game based on the given arguments.
 * It checks if the command exists in the list of available commands and
 * executes it. If the command is not found, it checks for specific commands
 * that can only be executed by a God player. If the command is not found and
 * the player is not a God, it displays an error message. After processing the
 * command, it computes the prompt and sends it to the player.
 *
 * @param g The GameObj representing the game state.
 * @param argv The command arguments.
 */
static void process_command(GameObj& g, const command_t& argv) {
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

/**
 * Ensure each player has a self-invite/self-pledge in their block
 */
static void initialize_block_data(EntityManager& entity_manager) {
  for (auto race_handle : RaceList(entity_manager)) {
    const auto& race = race_handle.read();
    const player_t i = race.Playernum;
    auto block_handle = entity_manager.get_block(i.value);
    setbit(block_handle->invite, i);
    setbit(block_handle->pledge, i);
  }
}
