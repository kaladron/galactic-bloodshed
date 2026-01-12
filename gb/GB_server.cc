// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import asio;
import commands;
import dallib;
import gblib;
import notification;
import session;
import std.compat;

#include <sys/stat.h>
#include <unistd.h>

#include <cctype>
#include <csignal>
#include <cstdio>
#include <cstdlib>

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

  time_t go_time_ = 0;
  std::chrono::steady_clock::time_point last_quota_update_;
};

bool shutdown_flag = false;  // Used by shutdown command

static void process_command(GameObj&, const command_t& argv);

static void GB_time(const command_t&, GameObj&);
static void GB_schedule(const command_t&, GameObj&);
static void initialize_block_data(EntityManager&);
static void welcome_user(Session&);
static void check_connect(Session&, std::string_view);

using CommandFunction = void (*)(const command_t&, GameObj&);

static const std::unordered_map<std::string, CommandFunction>& getCommands() {
  static std::unordered_map<std::string, CommandFunction> commands{
      {"'", GB::commands::announce},
      {"allocate", allocateAPs},
      {"analysis", GB::commands::analysis},
      {"announce", GB::commands::announce},
      {"appoint", GB::commands::governors},
      {"assault", GB::commands::dock},
      {"arm", GB::commands::arm},
      {"autoreport", GB::commands::autoreport},
      {"bless", GB::commands::bless},
      {"block", GB::commands::block},
      {"bombard", GB::commands::bombard},  // TODO(jeffbailey): !guest
      {"broadcast", GB::commands::announce},
      {"build", GB::commands::build},
      {"capital", GB::commands::capital},
      {"capture", GB::commands::capture},
      {"center", GB::commands::center},
      {"cew", GB::commands::fire},
      {"client_survey", GB::commands::survey},
      {"colonies", GB::commands::colonies},
      {"cs", GB::commands::cs},
      {"declare", GB::commands::declare},
      {"deploy", GB::commands::move_popn},
      {"detonate", GB::commands::detonate},  // TODO(jeffbailey): !guest
      {"disarm", GB::commands::arm},
      {"dismount", GB::commands::mount},
      {"dissolve", GB::commands::dissolve},  // TODO(jeffbailey): !guest
      {"distance", GB::commands::distance},
      {"dock", GB::commands::dock},
      {"dump", GB::commands::dump},
      {"enslave", GB::commands::enslave},
      {"examine", GB::commands::examine},
      {"explore", GB::commands::explore},
      {"factories", GB::commands::rst},
      {"fire", GB::commands::fire},  // TODO(jeffbailey): !guest
      {"fix", GB::commands::fix},
      {"fuel", GB::commands::proj_fuel},
      {"give", GB::commands::give},  // TODO(jeffbailey): !guest
      {"governors", GB::commands::governors},
      {"grant", GB::commands::grant},
      {"help", GB::commands::help},
      {"highlight", GB::commands::highlight},
      {"identify", GB::commands::whois},
      {"invite", GB::commands::invite},
      {"jettison", GB::commands::jettison},
      {"land", GB::commands::land},
      {"launch", GB::commands::launch},
      {"load", GB::commands::load},
      {"make", GB::commands::make_mod},
      {"map", GB::commands::map},
      {"mobilize", GB::commands::mobilize},
      {"modify", GB::commands::make_mod},
      {"move", GB::commands::move_popn},
      {"mount", GB::commands::mount},
      {"motto", GB::commands::motto},
      {"name", GB::commands::name},
      {"orbit", GB::commands::orbit},
      {"order", GB::commands::order},
      {"page", GB::commands::page},
      {"pay", GB::commands::pay},  // TODO(jeffbailey): !guest
      {"personal", GB::commands::personal},
      {"pledge", GB::commands::pledge},
      {"power", GB::commands::power},
      {"profile", GB::commands::profile},
      {"post", GB::commands::send_message},
      {"production", GB::commands::production},
      {"purge", GB::commands::purge},
      {"quit", GB::commands::quit},
      {"relation", GB::commands::relation},
      {"read", GB::commands::read_messages},
      {"repair", GB::commands::repair},
      {"report", GB::commands::rst},
      {"revoke", GB::commands::governors},
      {"route", GB::commands::route},
      {"schedule", GB_schedule},
      {"scrap", GB::commands::scrap},
      {"@@shutdown", GB::commands::shutdown},
      {"send", GB::commands::send_message},
      {"shout", GB::commands::announce},
      {"survey", GB::commands::survey},
      {"ship", GB::commands::rst},
      {"stars", GB::commands::star_locations},
      {"stats", GB::commands::rst},
      {"status", GB::commands::tech_status},
      {"stock", GB::commands::rst},
      {"tactical", GB::commands::tactical},
      {"technology", GB::commands::technology},
      {"think", GB::commands::announce},
      {"time", GB_time},
      {"toggle", GB::commands::toggle},
      {"toxicity", GB::commands::toxicity},
      {"transfer", GB::commands::transfer},
      {"undock", GB::commands::launch},
      {"uninvite", GB::commands::invite},
      {"unload", GB::commands::load},
      {"unpledge", GB::commands::unpledge},
      {"upgrade", GB::commands::upgrade},
      {"victory", GB::commands::victory},
      {"walk", GB::commands::walk},
      {"whois", GB::commands::whois},
      {"weapons", GB::commands::rst},
      {"zoom", GB::commands::zoom},
  };

  if (VOTING) {
    commands["vote"] = GB::commands::vote;
  }

  if (DEFENSE) {
    commands["defend"] = GB::commands::defend;
  }

  if (MARKET) {
    commands["bid"] = GB::commands::bid;
    commands["insurgency"] = GB::commands::insurgency;
    commands["sell"] = GB::commands::sell;
    commands["tax"] = GB::commands::tax;
    commands["treasury"] = GB::commands::treasury;
  }

  return commands;
}

namespace {
command_t make_command_t(std::string_view message) {
  command_t argv;

  size_t position;
  while ((position = message.find(' ')) != std::string_view::npos) {
    if (position == 0) {
      message.remove_prefix(1);
      continue;
    }
    argv.emplace_back(message.substr(0, position));
    message.remove_prefix(position + 1);
  }

  if (!message.empty()) argv.emplace_back(message);

  return argv;
}

struct connection_password {
  std::string player;
  std::string governor;
};
/**
 * \brief Parse input string for player and governor password
 * \param message Input string from the user
 * \return player and governor password or empty strings if invalid
 */
connection_password parse_connect(const std::string_view message) {
  auto argv = make_command_t(message);

  if (argv.size() != 2) {
    return {"", ""};
  }

  return {argv[0], argv[1]};
}

/**
 * \brief Create a prompt that shows the current AP and location of the player
 * \param g Game Object with player information
 * \return Prompt string for display to the user
 */
std::string do_prompt(GameObj& g) {
  player_t Playernum = g.player();
  std::stringstream prompt;

  const auto* universe = g.entity_manager.peek_universe();
  switch (g.level()) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [{0}] / )\n",
                            universe->AP[Playernum.value - 1]);
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      auto star = g.entity_manager.get_star(g.snum());
      const auto& star_ref = star.read();
      prompt << std::format(" ( [{0}] /{1} )\n", star_ref.AP(Playernum),
                            star_ref.get_name());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      auto star = g.entity_manager.get_star(g.snum());
      const auto& star_ref = star.read();
      prompt << std::format(" ( [{0}] /{1}/{2} )\n", star_ref.AP(Playernum),
                            star_ref.get_name(),
                            star_ref.get_planet_name(g.pnum()));
      return prompt.str();
    }
    case ScopeLevel::LEVEL_SHIP:
      break;  // That's the rest of this function.
  }

  const auto* s = g.entity_manager.peek_ship(g.shipno());
  if (!s) return " ( [?] /#? )\n";
  switch (s->whatorbits()) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [[0]] /#{1} )\n",
                            universe->AP[Playernum.value - 1], g.shipno());
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      auto star = g.entity_manager.get_star(s->storbits());
      const auto& star_ref = star.read();
      prompt << std::format(" ( [{0}] /{1}/#{2} )\n", star_ref.AP(Playernum),
                            star_ref.get_name(), g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      auto star = g.entity_manager.get_star(s->storbits());
      const auto& star_ref = star.read();
      prompt << std::format(" ( [{0}] /{1}/{2}/#{3} )\n",
                            star_ref.AP(Playernum), star_ref.get_name(),
                            star_ref.get_planet_name(g.pnum()), g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_SHIP:
      break;  // That's the rest of this function.  (Ship within a ship)
  }

  /* I put this mess in because of non-functioning prompts when you
     are in a ship within a ship, or deeper. I am certain this can be
     done more elegantly (a lot more) but I don't feel like trying
     that right now. right now I want it to function. Maarten */
  const auto* s2 = g.entity_manager.peek_ship(s->destshipno());
  if (!s2) return " ( [?] /#?/#? )\n";
  switch (s2->whatorbits()) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [{0}] /#{1}/#{2} )\n",
                            universe->AP[Playernum.value - 1], s->destshipno(),
                            g.shipno());
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      if (!star) return " ( [?] /?/#?/#? )\n";
      prompt << std::format(" ( [{0}] /{1}/#{2}/#{3} )\n", star->AP(Playernum),
                            star->get_name(), s->destshipno(), g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      if (!star) return " ( [?] /?/?/#?/#? )\n";
      prompt << std::format(" ( [{0}] /{1}/{2}/#{3}/#{4} )\n",
                            star->AP(Playernum), star->get_name(),
                            star->get_planet_name(g.pnum()), s->destshipno(),
                            g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_SHIP:
      break;  // That's the rest of this function.  (Ship w/in ship w/in ship)
  }

  while (s2->whatorbits() == ScopeLevel::LEVEL_SHIP) {
    s2 = g.entity_manager.peek_ship(s2->destshipno());
    if (!s2) return " ( [?] / /../#?/#? )\n";
  }
  switch (s2->whatorbits()) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [{0}] / /../#{1}/#{2} )\n",
                            universe->AP[Playernum.value - 1], s->destshipno(),
                            g.shipno());
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      if (!star) return " ( [?] /?/ /../#?/#? )\n";
      prompt << std::format(" ( [{0}] /{1}/ /../#{2}/#{3} )\n",
                            star->AP(Playernum), star->get_name(),
                            s->destshipno(), g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      if (!star) return " ( [?] /?/?/ /../#?/#? )\n";
      prompt << std::format(" ( [{0}] /{1}/{2}/ /../#{3}/#{4} )\n",
                            star->AP(Playernum), star->get_name(),
                            star->get_planet_name(g.pnum()), s->destshipno(),
                            g.shipno());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_SHIP:
      break;  // (Ship w/in ship w/in ship w/in ship)
  }
  // Kidding!  All done. =)
  return prompt.str();
}
}  // namespace

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
        welcome_user(*session);
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
  time_t current_time = std::time(nullptr);
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
  time_t now = std::time(nullptr);
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

  std::println("      ***   Galactic Bloodshed ver {0} ***", GB_VERSION);
  std::println();
  time_t clk = time(nullptr);
  std::print("      {0}", ctime(&clk));
  if (EXTERNAL_TRIGGER) {
    std::println("      The update  password is '%s'.", UPDATE_PASSWORD);
    std::println("      The segment password is '%s'.", SEGMENT_PASSWORD);
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
  std::print(stderr, "Last Update {:3d} : {}", 0, ctime(&clk));
  std::print(stderr, "Last Segment {0:2d} : {1}", state.nsegments_done,
             ctime(&clk));
  srandom(getpid());
  std::print(stderr, "      Next Update {0}  : {1}", 1,
             ctime(&state.next_update_time));
  std::print(stderr, "      Next Segment   : {0}",
             ctime(&state.next_segment_time));

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

  std::println("Going down.");
  return 0;
}

static void welcome_user(Session& session) {
  session.out() << std::format("***   Welcome to Galactic Bloodshed {} ***\n"
                               "Please enter your password:\n",
                               GB_VERSION);

  if (auto f = std::ifstream(WELCOME_FILE)) {
    std::string line;
    while (std::getline(f, line)) {
      session.out() << line << "\n";
    }
  }

  // Immediately flush welcome message (before command loop starts)
  session.flush_to_network();
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

static void check_connect(Session& session, std::string_view message) {
  auto [race_password, gov_password] = parse_connect(message);

  if (EXTERNAL_TRIGGER) {
    if (race_password == SEGMENT_PASSWORD) {
      do_segment(session.entity_manager(), session.registry(), 1, 0);
      return;
    } else if (race_password == UPDATE_PASSWORD) {
      do_update(session.entity_manager(), session.registry(), true);
      return;
    }
  }

  auto [Playernum, Governor] =
      getracenum(session.entity_manager(), race_password, gov_password);

  if (Playernum == 0) {
    session.out() << "Connection refused.\n";
    std::println(stderr, "FAILED CONNECT {},{}\n", race_password, gov_password);
    return;
  }

  auto race_handle = session.entity_manager().get_race(Playernum);
  if (!race_handle.get()) {
    session.out() << "Connection refused.\n";
    return;
  }
  const auto& race = race_handle.read();

  // Check if player is already connected
  if (session.registry().is_connected(Playernum, Governor)) {
    session.out() << "Connection refused.\n";
    return;
  }

  std::print(stderr, "CONNECTED {} \"{}\" [{},{}]\\n", race.name,
             race.governor[Governor.value].name, Playernum, Governor);
  session.set_connected(true);
  session.set_god(race.God);
  session.set_player(Playernum);
  session.set_governor(Governor);

  // Initialize scope to default or safe values
  session.set_level(race.governor[Governor.value].deflevel);
  session.set_snum(race.governor[Governor.value].defsystem);
  session.set_pnum(race.governor[Governor.value].defplanetnum);
  session.set_shipno(0);

  // Validate and clamp star number
  const auto* universe = session.entity_manager().peek_universe();
  if (session.snum() >= universe->numstars) {
    session.set_snum(0);  // Default to first star if invalid
  }

  // Validate and clamp planet number
  const auto* init_star = session.entity_manager().peek_star(session.snum());
  if (init_star && session.pnum() >= init_star->numplanets()) {
    session.set_pnum(0);  // Default to first planet if invalid
  }

  // Send login messages
  session.out() << std::format("\n{} \"{}\" [{},{}] logged on.\n", race.name,
                               race.governor[Governor.value].name, Playernum,
                               Governor);
  session.out() << std::format(
      "You are {}.\n",
      race.governor[Governor.value].toggle.invisible ? "invisible" : "visible");

  // Display time
  GameObj temp_g(session.entity_manager(), session.registry());
  temp_g.set_player(Playernum);
  temp_g.set_governor(Governor);
  temp_g.race = session.entity_manager().peek_race(Playernum);
  GB_time({}, temp_g);

  session.out() << std::format("\nLast login      : {}",
                               ctime(&(race.governor[Governor.value].login)));

  // Update login time
  auto& race_mut = *race_handle;
  race_mut.governor[Governor.value].login = time(nullptr);

  if (!race.Gov_ship) {
    session.out()
        << "You have no Governmental Center.  No action points will be "
           "produced\nuntil you build one and designate a capital.\n";
  } else {
    session.out() << std::format("Government Center #{} is active.\n",
                                 race.Gov_ship);
  }
  session.out() << std::format("     Morale: {}\n", race.morale);

  GB::commands::treasury({}, temp_g);

  // Flush temp_g output to session
  session.out() << temp_g.out.str();
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
  const auto* race = g.entity_manager.peek_race(g.player());
  if (!race) {
    g.out << "Error: Could not find your race.\n";
    return;
  }
  g.race = race;

  const auto& commands = getCommands();
  auto command = commands.find(argv[0]);
  if (command != commands.end()) {
    command->second(argv, g);
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

/* report back the update status */
static void GB_time(const command_t&, GameObj& g) {
  time_t clk = time(nullptr);
  const auto* state = g.entity_manager.peek_server_state();
  if (!state) {
    g.out << "Server state unavailable.\n";
    return;
  }
  const auto& sched = get_schedule_info();
  g.out << sched.start_buf;
  g.out << sched.update_buf;
  g.out << sched.segment_buf;
  g.out << std::format("Current time    : {0}", ctime(&clk));
}

static void GB_schedule(const command_t&, GameObj& g) {
  time_t clk = time(nullptr);
  const auto* state = g.entity_manager.peek_server_state();
  if (!state) {
    g.out << "Server state unavailable.\n";
    return;
  }
  const auto& sched = get_schedule_info();
  g.out << std::format("{0} minute update intervals\n",
                       state->update_time_minutes);
  g.out << std::format("{0} movement segments per update\n", state->segments);
  g.out << std::format("Current time    : {0}", ctime(&clk));
  g.out << std::format(
      "Next Segment {0:2d} : {1}",
      state->nsegments_done == state->segments ? 1 : state->nsegments_done + 1,
      ctime(&state->next_segment_time));
  g.out << std::format("Next Update {0:3d} : {1}", sched.nupdates_done + 1,
                       ctime(&state->next_update_time));
}
