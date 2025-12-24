// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import commands;
import dallib;
import gblib;
import std.compat;

#include "gb/GB_server.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>

#include "gb/files.h"

static bool shutdown_flag = false;

static time_t last_update_time;
static time_t last_segment_time;
static unsigned int nupdates_done; /* number of updates so far */

static std::string start_buf;
static std::string update_buf;
static std::string segment_buf;

static double GetComplexity(const ShipType);
static void set_signals();
static void help(const command_t&, GameObj&);
static void process_command(GameObj&, const command_t& argv);
static int shovechars(int, EntityManager&);

static void GB_time(const command_t&, GameObj&);
static void GB_schedule(const command_t&, GameObj&);
static void do_update(EntityManager&, bool = false);
static void do_segment(EntityManager&, int, int);
static int make_socket(int);
static void shutdownsock(DescriptorData&);
static void initialize_block_data(EntityManager&);
static void make_nonblocking(int);
static struct timeval update_quotas(struct timeval, struct timeval);
static bool process_output(DescriptorData&);
static void welcome_user(DescriptorData&);
static void process_commands();
static bool do_command(DescriptorData&, std::string_view);
static void goodbye_user(DescriptorData&);
static void dump_users(DescriptorData&);
static void close_sockets(int, EntityManager&);
static bool process_input(DescriptorData&);
static void force_output();
static void help_user(GameObj&);
static int msec_diff(struct timeval, struct timeval);
static struct timeval msec_add(struct timeval, int);
static void save_command(DescriptorData&, const std::string&);
static int ShipCompare(const void*, const void*);
static void SortShips();

static void check_connect(DescriptorData&, std::string_view);
static struct timeval timeval_sub(struct timeval now, struct timeval then);

constexpr int MAX_COMMAND_LEN = 512;

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
      {"help", help},
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
      {"relation", GB::commands::relation},
      {"read", GB::commands::read_messages},
      {"repair", GB::commands::repair},
      {"report", GB::commands::rst},
      {"revoke", GB::commands::governors},
      {"route", GB::commands::route},
      {"schedule", GB_schedule},
      {"scrap", GB::commands::scrap},
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
  player_t Playernum = g.player;
  std::stringstream prompt;

  const auto* universe = g.entity_manager.peek_universe();
  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [{0}] / )\n", universe->AP[Playernum - 1]);
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      auto star = g.entity_manager.get_star(g.snum);
      const auto& star_ref = star.read();
      prompt << std::format(" ( [{0}] /{1} )\n", star_ref.AP(Playernum - 1),
                            star_ref.get_name());
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      auto star = g.entity_manager.get_star(g.snum);
      const auto& star_ref = star.read();
      prompt << std::format(" ( [{0}] /{1}/{2} )\n", star_ref.AP(Playernum - 1),
                            star_ref.get_name(),
                            star_ref.get_planet_name(g.pnum));
      return prompt.str();
    }
    case ScopeLevel::LEVEL_SHIP:
      break;  // That's the rest of this function.
  }

  const auto* s = g.entity_manager.peek_ship(g.shipno);
  if (!s) return " ( [?] /#? )\n";
  switch (s->whatorbits()) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [[0]] /#{1} )\n", universe->AP[Playernum - 1],
                            g.shipno);
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      auto star = g.entity_manager.get_star(s->storbits());
      const auto& star_ref = star.read();
      prompt << std::format(" ( [{0}] /{1}/#{2} )\n",
                            star_ref.AP(Playernum - 1), star_ref.get_name(),
                            g.shipno);
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      auto star = g.entity_manager.get_star(s->storbits());
      const auto& star_ref = star.read();
      prompt << std::format(" ( [{0}] /{1}/{2}/#{3} )\n",
                            star_ref.AP(Playernum - 1), star_ref.get_name(),
                            star_ref.get_planet_name(g.pnum), g.shipno);
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
                            universe->AP[Playernum - 1], s->destshipno(),
                            g.shipno);
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      if (!star) return " ( [?] /?/#?/#? )\n";
      prompt << std::format(" ( [{0}] /{1}/#{2}/#{3} )\n",
                            star->AP(Playernum - 1), star->get_name(),
                            s->destshipno(), g.shipno);
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      if (!star) return " ( [?] /?/?/#?/#? )\n";
      prompt << std::format(" ( [{0}] /{1}/{2}/#{3}/#{4} )\n",
                            star->AP(Playernum - 1), star->get_name(),
                            star->get_planet_name(g.pnum), s->destshipno(),
                            g.shipno);
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
                            universe->AP[Playernum - 1], s->destshipno(),
                            g.shipno);
      return prompt.str();
    case ScopeLevel::LEVEL_STAR: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      if (!star) return " ( [?] /?/ /../#?/#? )\n";
      prompt << std::format(" ( [{0}] /{1}/ /../#{2}/#{3} )\n",
                            star->AP(Playernum - 1), star->get_name(),
                            s->destshipno(), g.shipno);
      return prompt.str();
    }
    case ScopeLevel::LEVEL_PLAN: {
      const auto* star = g.entity_manager.peek_star(s->storbits());
      if (!star) return " ( [?] /?/?/ /../#?/#? )\n";
      prompt << std::format(" ( [{0}] /{1}/{2}/ /../#{3}/#{4} )\n",
                            star->AP(Playernum - 1), star->get_name(),
                            star->get_planet_name(g.pnum), s->destshipno(),
                            g.shipno);
      return prompt.str();
    }
    case ScopeLevel::LEVEL_SHIP:
      break;  // (Ship w/in ship w/in ship w/in ship)
  }
  // Kidding!  All done. =)
  return prompt.str();
}
}  // namespace

int main(int argc, char** argv) {
  struct stat stbuf;

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
  start_buf = std::format("Server started  : {0}", ctime(&clk));

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
  update_buf = std::format("Last Update {:3d} : {}", nupdates_done,
                           ctime(&last_update_time));
  segment_buf = std::format("Last Segment {0:2d} : {1}", state.nsegments_done,
                            ctime(&last_segment_time));

  std::print(stderr, "{}", update_buf);
  std::print(stderr, "{}", segment_buf);
  srandom(getpid());
  std::print(stderr, "      Next Update {0}  : {1}", nupdates_done + 1,
             ctime(&state.next_update_time));
  std::print(stderr, "      Next Segment   : {0}",
             ctime(&state.next_segment_time));

  // Initialize game data structures
  initialize_block_data(entity_manager);  // Ensure self-invite/self-pledge
  compute_power_blocks(entity_manager);   // Calculate alliance power stats
  SortShips();  // Sort ship list by tech for "build ?"

  // Start server
  set_signals();
  int sock = shovechars(port, entity_manager);

  // Save final state before shutdown
  server_state_handle.save();

  // Shutdown
  close_sockets(sock, entity_manager);
  std::println("Going down.");
  return 0;
}

static void set_signals() {
  signal(SIGPIPE, SIG_IGN);
}

static struct timeval timeval_sub(struct timeval now, struct timeval then) {
  now.tv_sec -= then.tv_sec;
  now.tv_usec -= then.tv_usec;
  if (now.tv_usec < 0) {
    now.tv_usec += 1000000;
    now.tv_sec--;
  }
  return now;
}

static int msec_diff(struct timeval now, struct timeval then) {
  return ((now.tv_sec - then.tv_sec) * 1000 +
          (now.tv_usec - then.tv_usec) / 1000);
}

static struct timeval msec_add(struct timeval t, int x) {
  t.tv_sec += x / 1000;
  t.tv_usec += (x % 1000) * 1000;
  if (t.tv_usec >= 1000000) {
    t.tv_sec += t.tv_usec / 1000000;
    t.tv_usec = t.tv_usec % 1000000;
  }
  return t;
}

static int shovechars(int port, EntityManager& entity_manager) {
  fd_set input_set;
  fd_set output_set;
  struct timeval last_slice;
  struct timeval current_time;
  struct timeval next_slice;
  struct timeval timeout;
  struct timeval slice_timeout;
  time_t now;
  time_t go_time = 0;

  int sock = make_socket(port);
  gettimeofday(&last_slice, nullptr);

  if (!shutdown_flag)
    post(entity_manager, "Server started\n", NewsType::ANNOUNCE);

  while (!shutdown_flag) {
    fflush(stdout);
    gettimeofday(&current_time, nullptr);
    last_slice = update_quotas(last_slice, current_time);

    process_commands();

    if (shutdown_flag) break;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    next_slice = msec_add(last_slice, COMMAND_TIME_MSEC);
    slice_timeout = timeval_sub(next_slice, current_time);

    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_SET(sock, &input_set);
    for (auto& d : descriptor_list) {
      if (!d.input.empty())
        timeout = slice_timeout;
      else
        FD_SET(d.descriptor, &input_set);
      // Is there anything in the output queue?
      if (!d.output.empty() || !d.out.str().empty())
        FD_SET(d.descriptor, &output_set);
    }

    if (select(FD_SETSIZE, &input_set, &output_set, nullptr, &timeout) < 0) {
      if (errno != EINTR) {
        perror("select");
        return sock;
      }
      (void)time(&now);
    } else {
      (void)time(&now);

      if (FD_ISSET(sock, &input_set)) {
        try {
          descriptor_list.emplace_back(sock, entity_manager);
          auto& newd = descriptor_list.back();
          make_nonblocking(newd.descriptor);
          welcome_user(newd);
        } catch (const std::runtime_error&) {
          perror("new_connection");
          return sock;
        }
      }

      // Use iterator loop to handle removal during iteration
      for (auto it = descriptor_list.begin(); it != descriptor_list.end();) {
        auto& d = *it;
        bool should_remove = false;

        if (FD_ISSET(d.descriptor, &input_set)) {
          /*      d->last_time = now; */
          if (!process_input(d)) {
            should_remove = true;
          }
        }

        if (!should_remove && FD_ISSET(d.descriptor, &output_set)) {
          if (!process_output(d)) {
            should_remove = true;
          }
        }

        if (should_remove) {
          shutdownsock(d);
          // shutdownsock removes the element, so iterator is invalidated
          // Restart from beginning since list structure changed
          it = descriptor_list.begin();
        } else {
          ++it;
        }
      }
    }
    if (go_time == 0) {
      const auto* state = entity_manager.peek_server_state();
      if (state) {
        if (now >= state->next_update_time) {
          go_time =
              now + (int_rand(0, DEFAULT_RANDOM_UPDATE_RANGE.count()) * 60);
        }
        if (now >= state->next_segment_time &&
            state->nsegments_done < state->segments) {
          go_time =
              now + (int_rand(0, DEFAULT_RANDOM_SEGMENT_RANGE.count()) * 60);
        }
      }
    }
    if (go_time > 0 && now >= go_time) {
      do_next_thing(entity_manager);
      go_time = 0;
    }
  }
  return sock;
}

void do_next_thing(EntityManager& entity_manager) {
  const auto* state = entity_manager.peek_server_state();
  if (!state) return;

  if (state->nsegments_done < state->segments)
    do_segment(entity_manager, 0, 1);
  else
    do_update(entity_manager, false);
}

static int make_socket(int port) {
  int s;
  struct sockaddr_in6 server;
  int opt;

  s = socket(AF_INET6, SOCK_STREAM, 0);
  if (s < 0) {
    perror("creating stream socket");
    exit(3);
  }
  opt = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(1);
  }
  if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, /* GVC */
                 (char*)&opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(1);
  }
  memset(&server, 0, sizeof(server));
  server.sin6_family = AF_INET6;
  server.sin6_addr = in6addr_any;
  server.sin6_port = htons(port);
  if (bind(s, (struct sockaddr*)&server, sizeof(server))) {
    perror("binding stream socket");
    close(s);
    exit(4);
  }
  listen(s, 5);
  return s;
}

static struct timeval update_quotas(struct timeval last,
                                    struct timeval current) {
  int nslices = msec_diff(current, last) / COMMAND_TIME_MSEC;

  if (nslices > 0) {
    for (auto& d : descriptor_list) {
      d.quota += COMMANDS_PER_TIME * nslices;
      if (d.quota > COMMAND_BURST_SIZE) d.quota = COMMAND_BURST_SIZE;
    }
  }
  return msec_add(last, nslices * COMMAND_TIME_MSEC);
}

static void shutdownsock(DescriptorData& d) {
  if (d.connected) {
    std::println(stderr, "DISCONNECT {0} Race={1} Governor={2}", d.descriptor,
                 d.player, d.governor);
  } else {
    std::println(stderr, "DISCONNECT {0} never connected", d.descriptor);
  }
  shutdown(d.descriptor, 2);
  close(d.descriptor);
  descriptor_list.remove(d);
}

static bool process_output(DescriptorData& d) {
  // Flush the stringstream buffer into the output queue.
  strstr_to_queue(d);

  while (!d.output.empty()) {
    auto& cur = d.output.front();
    ssize_t cnt = write(d.descriptor, cur.c_str(), cur.size());
    if (cnt < 0) {
      if (errno == EWOULDBLOCK) return true;
      d.connected = false;
      return false;
    }
    d.output_size -= cnt;
    if (cnt == cur.size()) {  // We output the entire block
      d.output.pop_front();
      continue;
    }
    // We only output part of it, so we don't clear it out.
    cur.erase(cnt);
    d.output.pop_front();
    d.output.push_front(cur);
    break;
  }
  return true;
}

static void force_output() {
  for (auto& d : descriptor_list)
    if (d.connected) (void)process_output(d);
}

static void make_nonblocking(int s) {
  if (fcntl(s, F_SETFL, O_NDELAY) == -1) {
    perror("make_nonblocking: fcntl");
    exit(0);
  }
}

static void welcome_user(DescriptorData& d) {
  FILE* f;
  char* p;

  std::string welcome_msg = std::format(
      "***   Welcome to Galactic Bloodshed {} ***\nPlease enter your "
      "password:\n",
      GB_VERSION);
  queue_string(d, welcome_msg);

  if ((f = fopen(WELCOME_FILE, "r")) != nullptr) {
    char line_buf[2047];
    while (fgets(line_buf, sizeof line_buf, f)) {
      for (p = line_buf; *p; p++)
        if (*p == '\n') {
          *p = '\0';
          break;
        }
      queue_string(d, line_buf);
      queue_string(d, "\n");
    }
    fclose(f);
  }
}

static void help_user(GameObj& g) {
  FILE* f;
  char* p;

  if ((f = fopen(HELP_FILE, "r")) != nullptr) {
    char line_buf[2047];
    while (fgets(line_buf, sizeof line_buf, f)) {
      for (p = line_buf; *p; p++)
        if (*p == '\n') {
          *p = '\0';
          break;
        }
      notify(g.player, g.governor, line_buf);
      notify(g.player, g.governor, "\n");
    }
    fclose(f);
  }
}

static void goodbye_user(DescriptorData& d) {
  if (d.connected) /* this can happen, especially after updates */
    if (write(d.descriptor, LEAVE_MESSAGE, strlen(LEAVE_MESSAGE)) < 0) {
      perror("write error");
      exit(-1);
    }
}

static void save_command(DescriptorData& d, const std::string& command) {
  add_to_queue(d.input, command);
}

static bool process_input(DescriptorData& d) {
  std::array<char, 2048> input_buf;

  ssize_t got = read(d.descriptor, input_buf.data(), input_buf.size());
  if (got <= 0) return false;

  for (ssize_t i = 0; i < got; ++i) {
    char c = input_buf[i];
    if (c == '\n') {
      if (!d.raw_input.empty()) {
        save_command(d, d.raw_input);
        d.raw_input.clear();
      }
    } else if (d.raw_input.size() < MAX_COMMAND_LEN - 1 &&
               std::isprint(static_cast<unsigned char>(c))) {
      d.raw_input += c;
    }
  }
  return true;
}

static void process_commands() {
  int nprocessed;
  time_t now;

  (void)time(&now);

  do {
    nprocessed = 0;
    for (auto& d : descriptor_list) {
      if (d.quota > 0 && !d.input.empty()) {
        auto& t = d.input.front();
        d.quota--;
        nprocessed++;

        if (!do_command(d, t)) {
          shutdownsock(d);
          break;
        }
        d.last_time = now; /* experimental code */
        d.input.pop_front();
        d.last_time = now; /* experimental code */
      }
    }
  } while (nprocessed > 0);
}

/** Main processing loop. When command strings are sent from the client,
   they are processed here. Responses are sent back to the client via
   notify.
   */
static bool do_command(DescriptorData& d, std::string_view comm) {
  /* check to see if there are a few words typed out, usually for the help
   * command */
  auto argv = make_command_t(comm);

  if (argv[0] == "quit") {
    goodbye_user(d);
    return false;
  }
  if (d.connected && argv[0] == "who") {
    dump_users(d);
  } else if (d.connected && d.god && argv[0] == "emulate") {
    d.player = std::stoi(argv[1]);
    d.governor = std::stoi(argv[2]);
    const auto* race = d.entity_manager.peek_race(d.player);
    if (race) {
      std::string emulate_msg =
          std::format("Emulating {} \"{}\" [{},{}]\n", race->name,
                      race->governor[d.governor].name, d.player, d.governor);
      queue_string(d, emulate_msg);
    }
  } else {
    if (d.connected) {
      /* GB command parser */
      process_command(d, argv);
    } else {
      check_connect(
          d, comm); /* Logs player into the game, connects
                          if the password given by *command is a player's */
      if (!d.connected) {
        goodbye_user(d);
      } else {
        check_for_telegrams(d);
        /* set the scope to home upon login */
        command_t call_cs = {"cs"};
        process_command(d, call_cs);
      }
    }
  }
  return true;
}

static void check_connect(DescriptorData& d, std::string_view message) {
  auto [race_password, gov_password] = parse_connect(message);

  if (EXTERNAL_TRIGGER) {
    if (race_password == SEGMENT_PASSWORD) {
      do_segment(d.entity_manager, 1, 0);
      return;
    } else if (race_password == UPDATE_PASSWORD) {
      do_update(d.entity_manager, true);
      return;
    }
  }

  auto [Playernum, Governor] =
      getracenum(d.entity_manager, race_password, gov_password);

  if (!Playernum) {
    queue_string(d, "Connection refused.\n");
    std::println(stderr, "FAILED CONNECT {0},{1} on descriptor {2}\n",
                 race_password.c_str(), gov_password.c_str(), d.descriptor);
    return;
  }

  auto race_handle = d.entity_manager.get_race(Playernum);
  if (!race_handle.get()) {
    queue_string(d, "Connection refused.\n");
    return;
  }
  const auto& race = race_handle.read();
  /* check to see if this player is already connected, if so, nuke the
   * descriptor */
  for (auto& d0 : descriptor_list) {
    if (d0.connected && d0.player == Playernum && d0.governor == Governor) {
      queue_string(d, "Connection refused.\n");
      return;
    }
  }

  std::print(stderr, "CONNECTED {0} \"{1}\" [{2},{3}] on descriptor {4}\n",
             race.name, race.governor[Governor].name, Playernum, Governor,
             d.descriptor);
  d.connected = true;

  d.god = race.God;
  d.player = Playernum;
  d.governor = Governor;
  d.race =
      d.entity_manager.peek_race(Playernum);  // Set race pointer for GameObj

  // Initialize scope to default or safe values
  d.level = race.governor[Governor].deflevel;
  d.snum = race.governor[Governor].defsystem;
  d.pnum = race.governor[Governor].defplanetnum;
  d.shipno = 0;

  // Validate and clamp star number
  const auto* universe = d.entity_manager.peek_universe();
  if (d.snum >= universe->numstars) {
    d.snum = 0;  // Default to first star if invalid
  }

  // Validate and clamp planet number
  const auto* init_star = d.entity_manager.peek_star(d.snum);
  if (init_star && d.pnum >= init_star->numplanets()) {
    d.pnum = 0;  // Default to first planet if invalid
  }

  std::string login_msg =
      std::format("\n{} \"{}\" [{},{}] logged on.\n", race.name,
                  race.governor[Governor].name, Playernum, Governor);
  notify_race(Playernum, login_msg);
  std::string visibility_msg = std::format(
      "You are {}.\n",
      race.governor[Governor].toggle.invisible ? "invisible" : "visible");
  notify(Playernum, Governor, visibility_msg);

  GB_time({}, d);
  std::string last_login_msg = std::format(
      "\nLast login      : {}", ctime(&(race.governor[Governor].login)));
  notify(Playernum, Governor, last_login_msg);
  auto& race_mut = *race_handle;
  race_mut.governor[Governor].login = time(nullptr);
  if (!race.Gov_ship) {
    std::string no_gov_msg =
        "You have no Governmental Center.  No action points will be "
        "produced\nuntil you build one and designate a capital.\n";
    notify(Playernum, Governor, no_gov_msg);
  } else {
    std::string gov_msg =
        std::format("Government Center #{} is active.\n", race.Gov_ship);
    notify(Playernum, Governor, gov_msg);
  }
  std::string morale_msg = std::format("     Morale: {}\n", race.morale);
  notify(Playernum, Governor, morale_msg);
  GB::commands::treasury({}, d);
}

static void do_update(EntityManager& entity_manager, bool force) {
  time_t clk = time(nullptr);
  struct stat stbuf;

  // Get server state handle (will auto-save on scope exit)
  auto state_handle = entity_manager.get_server_state();
  auto& state = *state_handle;

  bool fakeit = (!force && stat(nogofl.data(), &stbuf) >= 0);

  std::string update_msg = std::format("{}DOING UPDATE...\n", ctime(&clk));
  if (!fakeit) {
    for (auto i = 1; i <= entity_manager.num_races(); i++)
      notify_race(i, update_msg);
    force_output();
  }

  if (state.segments <= 1) {
    /* Disables movement segments. */
    state.next_segment_time = clk + (144 * 3600);
    state.nsegments_done = state.segments;
  } else {
    if (force)
      state.next_segment_time =
          clk + state.update_time_minutes * 60 / state.segments;
    else
      state.next_segment_time = state.next_update_time +
                                state.update_time_minutes * 60 / state.segments;
    state.nsegments_done = 1;
  }
  if (force)
    state.next_update_time = clk + state.update_time_minutes * 60;
  else
    state.next_update_time += state.update_time_minutes * 60;

  if (!fakeit) nupdates_done++;

  Power_blocks.time = clk;
  update_buf =
      std::format("Last Update {0:3d} : {1}", nupdates_done, ctime(&clk));
  std::print(stderr, "{}", ctime(&clk));
  std::print(stderr, "Next Update {0:3d} : {1}", nupdates_done + 1,
             ctime(&state.next_update_time));
  segment_buf = std::format("Last Segment {0:2d} : {1}", state.nsegments_done,
                            ctime(&clk));
  std::print(stderr, "{}", ctime(&clk));
  std::print(stderr, "Next Segment {0:2d} : {1}",
             state.nsegments_done == state.segments ? 1
                                                    : state.nsegments_done + 1,
             ctime(&state.next_segment_time));

  update_flag = true;
  if (!fakeit) do_turn(entity_manager, 1);
  update_flag = false;
  clk = time(nullptr);
  std::string finish_msg =
      std::format("{}Update {} finished\n", ctime(&clk), nupdates_done);
  handle_victory(entity_manager);
  if (!fakeit) {
    for (auto i = 1; i <= entity_manager.num_races(); i++)
      notify_race(i, finish_msg);
    force_output();
  }
}

static void do_segment(EntityManager& entity_manager, int override,
                       int segment) {
  time_t clk = time(nullptr);
  struct stat stbuf;

  // Get server state handle (will auto-save on scope exit)
  auto state_handle = entity_manager.get_server_state();
  auto& state = *state_handle;

  bool fakeit = (!override && stat(nogofl.data(), &stbuf) >= 0);

  if (!override && state.segments <= 1) return;

  std::string movement_msg = std::format("{}DOING MOVEMENT...\n", ctime(&clk));
  if (!fakeit) {
    for (auto i = 1; i <= entity_manager.num_races(); i++)
      notify_race(i, movement_msg);
    force_output();
  }
  if (override) {
    state.next_segment_time =
        clk + state.update_time_minutes * 60 / state.segments;
    if (segment) {
      state.nsegments_done = segment;
      state.next_update_time = clk + state.update_time_minutes * 60 *
                                         (state.segments - segment + 1) /
                                         state.segments;
    } else {
      state.nsegments_done++;
    }
  } else {
    state.next_segment_time += state.update_time_minutes * 60 / state.segments;
    state.nsegments_done++;
  }

  update_flag = true;
  if (!fakeit) do_turn(entity_manager, 0);
  update_flag = false;
  segment_buf = std::format("Last Segment {0:2d} : {1}", state.nsegments_done,
                            ctime(&clk));
  std::print(stderr, "{0}", ctime(&clk));
  std::print(stderr, "Next Segment {0:2d} : {1}", state.nsegments_done,
             ctime(&state.next_segment_time));
  clk = time(nullptr);
  std::string segment_msg = std::format("{}Segment finished\n", ctime(&clk));
  if (!fakeit) {
    for (auto i = 1; i <= entity_manager.num_races(); i++)
      notify_race(i, segment_msg);
    force_output();
  }
}

static void close_sockets(int sock, EntityManager& entity_manager) {
  /* post message into news file */
  const char* shutdown_message = "Shutdown ordered by deity - Bye\n";
  post(entity_manager, shutdown_message, NewsType::ANNOUNCE);

  for (auto& d : descriptor_list) {
    if (write(d.descriptor, shutdown_message, strlen(shutdown_message)) < 0) {
      perror("write error");
      exit(-1);
    }
    if (shutdown(d.descriptor, 2) < 0) perror("shutdown");
    close(d.descriptor);
  }
  close(sock);
}

static void dump_users(DescriptorData& e) {
  time_t now;
  int God = 0;
  int coward_count = 0;

  (void)time(&now);
  std::string players_msg = std::format("Current Players: {}", ctime(&now));
  queue_string(e, players_msg);
  if (e.player) {
    const auto* r = e.entity_manager.peek_race(e.player);
    if (!r) return;
    God = r->God;
  } else
    return;

  for (auto& d : descriptor_list) {
    if (d.connected && !d.god) {
      const auto* r = d.entity_manager.peek_race(d.player);
      if (!r) continue;
      if (!r->governor[d.governor].toggle.invisible || e.player == d.player ||
          God) {
        std::string temp = std::format("\"{}\"", r->governor[d.governor].name);
        const auto& star = *d.entity_manager.peek_star(d.snum);
        std::string user_info = std::format(
            "{:20.20s} {:20.20s} [{:2d},{:2d}] {:4d}s idle {:4.4s} {} {}\n",
            r->name, temp, d.player, d.governor, now - d.last_time,
            God ? star.get_name() : "    ",
            (r->governor[d.governor].toggle.gag ? "GAG" : "   "),
            (r->governor[d.governor].toggle.invisible ? "INVISIBLE" : ""));
        queue_string(e, user_info);
      } else if (!God) /* deity lurks around */
        coward_count++;

      if ((now - d.last_time) > DISCONNECT_TIME) d.connected = false;
    }
  }
  if (SHOW_COWARDS) {
    std::string coward_msg = std::format("And {} coward{}.\n", coward_count,
                                         (coward_count == 1) ? "" : "s");
    queue_string(e, coward_msg);
  } else {
    queue_string(e, "Finished.\n");
  }
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
  const auto* race = g.entity_manager.peek_race(g.player);
  if (!race) {
    g.out << "Error: Could not find your race.\n";
    return;
  }
  g.race = race;

  bool God = g.race->God;

  const auto& commands = getCommands();
  auto command = commands.find(argv[0]);
  if (command != commands.end()) {
    command->second(argv, g);
  } else if (argv[0] == "purge" && God)
    purge(g.entity_manager);
  else if (argv[0] == "@@shutdown" && God) {
    shutdown_flag = true;
    g.out << "Doing shutdown.\n";
  } else if (argv[0] == "@@update" && God)
    do_update(g.entity_manager, true);
  else if (argv[0] == "@@segment" && God)
    do_segment(g.entity_manager, 1, std::stoi(argv[1]));
  else {
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
    auto block_handle = entity_manager.get_block(i);
    setbit(block_handle->invite, i - 1);
    setbit(block_handle->pledge, i - 1);
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
  g.out << start_buf;
  g.out << update_buf;
  g.out << segment_buf;
  g.out << std::format("Current time    : {0}", ctime(&clk));
}

static void GB_schedule(const command_t&, GameObj& g) {
  time_t clk = time(nullptr);
  const auto* state = g.entity_manager.peek_server_state();
  if (!state) {
    g.out << "Server state unavailable.\n";
    return;
  }
  g.out << std::format("{0} minute update intervals\n",
                       state->update_time_minutes);
  g.out << std::format("{0} movement segments per update\n", state->segments);
  g.out << std::format("Current time    : {0}", ctime(&clk));
  g.out << std::format(
      "Next Segment {0:2d} : {1}",
      state->nsegments_done == state->segments ? 1 : state->nsegments_done + 1,
      ctime(&state->next_segment_time));
  g.out << std::format("Next Update {0:3d} : {1}", nupdates_done + 1,
                       ctime(&state->next_update_time));
}

static void help(const command_t& argv, GameObj& g) {
  FILE* f;
  char file[1024];
  char* p;

  if (argv.size() == 1) {
    help_user(g);
  } else {
    sprintf(file, "%s/%s.md", HELPDIR, argv[1].c_str());
    if ((f = fopen(file, "r")) != nullptr) {
      char help_buf[2047];
      while (fgets(help_buf, sizeof help_buf, f)) {
        for (p = help_buf; *p; p++)
          if (*p == '\n') {
            *p = '\0';
            break;
          }
        strcat(help_buf, "\n");
        notify(g.player, g.governor, help_buf);
      }
      fclose(f);
      notify(g.player, g.governor, "----\nFinished.\n");
    } else
      notify(g.player, g.governor, "Help on that subject unavailable.\n");
  }
}

void compute_power_blocks(EntityManager& entity_manager) {
  /* compute alliance block power */
  Power_blocks.time = time(nullptr);
  for (auto race_i_handle : RaceList(entity_manager)) {
    const auto& race_i = race_i_handle.read();
    const player_t i = race_i.Playernum;

    const auto* block_i = entity_manager.peek_block(i);
    if (!block_i) continue;

    uint64_t allied_members = block_i->invite & block_i->pledge;
    Power_blocks.members[i - 1] = 0;
    Power_blocks.sectors_owned[i - 1] = 0;
    Power_blocks.popn[i - 1] = 0;
    Power_blocks.ships_owned[i - 1] = 0;
    Power_blocks.resource[i - 1] = 0;
    Power_blocks.fuel[i - 1] = 0;
    Power_blocks.destruct[i - 1] = 0;
    Power_blocks.money[i - 1] = 0;
    Power_blocks.systems_owned[i - 1] = block_i->systems_owned;
    Power_blocks.VPs[i - 1] = block_i->VPs;

    for (auto race_j_handle : RaceList(entity_manager)) {
      const auto& race_j = race_j_handle.read();
      const player_t j = race_j.Playernum;

      if (isset(allied_members, j)) {
        const auto* power_ptr = entity_manager.peek_power(j);
        if (!power_ptr) continue;
        Power_blocks.members[i - 1] += 1;
        Power_blocks.sectors_owned[i - 1] += power_ptr->sectors_owned;
        Power_blocks.money[i - 1] += power_ptr->money;
        Power_blocks.popn[i - 1] += power_ptr->popn;
        Power_blocks.ships_owned[i - 1] += power_ptr->ships_owned;
        Power_blocks.resource[i - 1] += power_ptr->resource;
        Power_blocks.fuel[i - 1] += power_ptr->fuel;
        Power_blocks.destruct[i - 1] += power_ptr->destruct;
      }
    }
  }
}

static double GetComplexity(const ShipType ship) {
  Ship s;

  s.armor() = Shipdata[ship][ABIL_ARMOR];
  s.guns() = Shipdata[ship][ABIL_PRIMARY] ? PRIMARY : GTYPE_NONE;
  s.primary() = Shipdata[ship][ABIL_GUNS];
  s.primtype() = shipdata_primary(ship);
  s.secondary() = Shipdata[ship][ABIL_GUNS];
  s.sectype() = shipdata_secondary(ship);
  s.max_crew() = Shipdata[ship][ABIL_MAXCREW];
  s.max_resource() = Shipdata[ship][ABIL_CARGO];
  s.max_hanger() = Shipdata[ship][ABIL_HANGER];
  s.max_destruct() = Shipdata[ship][ABIL_DESTCAP];
  s.max_fuel() = Shipdata[ship][ABIL_FUELCAP];
  s.max_speed() = Shipdata[ship][ABIL_SPEED];
  s.build_type() = ship;
  s.mount() = Shipdata[ship][ABIL_MOUNT];
  s.hyper_drive().has = Shipdata[ship][ABIL_JUMP];
  s.cloak() = 0;
  s.laser() = Shipdata[ship][ABIL_LASER];
  s.cew() = 0;
  s.cew_range() = 0;
  s.size() = ship_size(s);
  s.base_mass() = getmass(s);
  s.mass() = getmass(s);

  return complexity(s);
}

static int ShipCompare(const void* S1, const void* S2) {
  const auto* s1 = (const ShipType*)S1;
  const auto* s2 = (const ShipType*)S2;
  return (int)(GetComplexity(*s1) - GetComplexity(*s2));
}

static void SortShips() {
  for (int i = 0; i < NUMSTYPES; i++)
    ShipVector[i] = i;
  qsort(ShipVector, NUMSTYPES, sizeof(int), ShipCompare);
}
