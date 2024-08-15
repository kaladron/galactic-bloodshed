// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

import commands;
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

#include "gb/buffers.h"
#include "gb/doturncmd.h"
#include "gb/files.h"
#include "gb/globals.h"
#include "gb/tweakables.h"

static int shutdown_flag = 0;

static time_t last_update_time;
static time_t last_segment_time;
static unsigned int nupdates_done; /* number of updates so far */

static std::string start_buf;
static std::string update_buf;
static std::string segment_buf;

static double GetComplexity(const ShipType);
static void set_signals();
static void help(const command_t &, GameObj &);
static void process_command(GameObj &, const command_t &argv);
static int shovechars(int, Db &);

static void GB_time(const command_t &, GameObj &);
static void GB_schedule(const command_t &, GameObj &);
static void do_update(Db &, bool = false);
static void do_segment(Db &, int, int);
static int make_socket(int);
static void shutdownsock(DescriptorData &);
static void load_race_data(Db &);
static void load_star_data();
static void make_nonblocking(int);
static struct timeval update_quotas(struct timeval, struct timeval);
static bool process_output(DescriptorData &);
static void welcome_user(DescriptorData &);
static void process_commands();
static bool do_command(DescriptorData &, std::string_view);
static void goodbye_user(DescriptorData &);
static void dump_users(DescriptorData &);
static void close_sockets(int);
static int process_input(DescriptorData &);
static void force_output();
static void help_user(GameObj &);
static int msec_diff(struct timeval, struct timeval);
static struct timeval msec_add(struct timeval, int);
static void save_command(DescriptorData &, const std::string &);
static int ShipCompare(const void *, const void *);
static void SortShips();

static void check_connect(DescriptorData &, std::string_view);
static struct timeval timeval_sub(struct timeval now, struct timeval then);

#define MAX_COMMAND_LEN 512

using CommandFunction = void (*)(const command_t &, GameObj &);

// NOLINTBEGIN
static const std::unordered_map<std::string, CommandFunction> commands{
    // NOLINTEND
    {"'", GB::commands::announce},
    {"allocate", allocateAPs},
    {"analysis", GB::commands::analysis},
    {"announce", GB::commands::announce},
    {"appoint", GB::commands::governors},
    {"assault", GB::commands::dock},
    {"arm", GB::commands::arm},
    {"autoreport", GB::commands::autoreport},
#ifdef MARKET
    {"bid", GB::commands::bid},
#endif
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
#ifdef DEFENSE
    {"defend", GB::commands::defend},
#endif
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
#ifdef MARKET
    {"insurgency", GB::commands::insurgency},
#endif
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
#ifdef MARKET
    {"sell", GB::commands::sell},
#endif
    {"send", GB::commands::send_message},
    {"shout", GB::commands::announce},
    {"survey", GB::commands::survey},
    {"ship", GB::commands::rst},
    {"stars", GB::commands::star_locations},
    {"stats", GB::commands::rst},
    {"status", GB::commands::tech_status},
    {"stock", GB::commands::rst},
    {"tactical", GB::commands::rst},
    {"technology", GB::commands::technology},
    {"think", GB::commands::announce},
    {"time", GB_time},
#ifdef MARKET
    {"tax", GB::commands::tax},
#endif
    {"toggle", GB::commands::toggle},
    {"toxicity", GB::commands::toxicity},
    {"transfer", GB::commands::transfer},
#ifdef MARKET
    {"treasury", GB::commands::treasury},
#endif
    {"undock", GB::commands::launch},
    {"uninvite", GB::commands::invite},
    {"unload", GB::commands::load},
    {"unpledge", GB::commands::unpledge},
    {"upgrade", GB::commands::upgrade},
    {"victory", GB::commands::victory},
#ifdef VOTING
    {"vote", GB::commands::vote},
#endif
    {"walk", GB::commands::walk},
    {"whois", GB::commands::whois},
    {"weapons", GB::commands::rst},
    {"zoom", GB::commands::zoom},
};

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

/**
 * \brief Parse input string for player and governor password
 * \param message Input string from the user
 * \return player and governor password or empty strings if invalid
 */
std::tuple<std::string, std::string> parse_connect(
    const std::string_view message) {
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
std::string do_prompt(GameObj &g) {
  player_t Playernum = g.player;
  std::stringstream prompt;

  switch (g.level) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [{0}] / )\n", Sdata.AP[Playernum - 1]);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_STAR:
      prompt << std::format(" ( [{0}] /{1} )\n",
                            stars[g.snum].AP[Playernum - 1],
                            stars[g.snum].name);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_PLAN:
      prompt << std::format(" ( [{0}] /{1}/{2} )\n",
                            stars[g.snum].AP[Playernum - 1], stars[g.snum].name,
                            stars[g.snum].pnames[g.pnum]);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_SHIP:
      break;  // That's the rest of this function.
  }

  auto s = getship(g.shipno);
  switch (s->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [[0]] /#{1} )\n", Sdata.AP[Playernum - 1],
                            g.shipno);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_STAR:
      prompt << std::format(" ( [{0}] /{1}/#{2} )\n",
                            stars[s->storbits].AP[Playernum - 1],
                            stars[s->storbits].name, g.shipno);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_PLAN:
      prompt << std::format(
          " ( [{0}] /{1}/{2}/#{3} )\n", stars[s->storbits].AP[Playernum - 1],
          stars[s->storbits].name, stars[s->storbits].pnames[g.pnum], g.shipno);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_SHIP:
      break;  // That's the rest of this function.  (Ship within a ship)
  }

  /* I put this mess in because of non-functioning prompts when you
     are in a ship within a ship, or deeper. I am certain this can be
     done more elegantly (a lot more) but I don't feel like trying
     that right now. right now I want it to function. Maarten */
  auto s2 = getship(s->destshipno);
  switch (s2->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [{0}] /#{1}/#{2} )\n", Sdata.AP[Playernum - 1],
                            s->destshipno, g.shipno);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_STAR:
      prompt << std::format(" ( [{0}] /{1}/#{2}/#{3} )\n",
                            stars[s->storbits].AP[Playernum - 1],
                            stars[s->storbits].name, s->destshipno, g.shipno);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_PLAN:
      prompt << std::format(
          " ( [{0}] /{1}/{2}/#{3}/#{4} )\n",
          stars[s->storbits].AP[Playernum - 1], stars[s->storbits].name,
          stars[s->storbits].pnames[g.pnum], s->destshipno, g.shipno);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_SHIP:
      break;  // That's the rest of this function.  (Ship w/in ship w/in ship)
  }

  while (s2->whatorbits == ScopeLevel::LEVEL_SHIP) {
    s2 = getship(s2->destshipno);
  }
  switch (s2->whatorbits) {
    case ScopeLevel::LEVEL_UNIV:
      prompt << std::format(" ( [{0}] / /../#{1}/#{2} )\n",
                            Sdata.AP[Playernum - 1], s->destshipno, g.shipno);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_STAR:
      prompt << std::format(" ( [{0}] /{1}/ /../#{2}/#{3} )\n",
                            stars[s->storbits].AP[Playernum - 1],
                            stars[s->storbits].name, s->destshipno, g.shipno);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_PLAN:
      prompt << std::format(
          " ( [{0}] /{1}/{2}/ /../#{3}/#{4} )\n",
          stars[s->storbits].AP[Playernum - 1], stars[s->storbits].name,
          stars[s->storbits].pnames[g.pnum], s->destshipno, g.shipno);
      prompt << std::ends;
      return prompt.str();
    case ScopeLevel::LEVEL_SHIP:
      break;  // (Ship w/in ship w/in ship w/in ship)
  }
  // Kidding!  All done. =)
  prompt << std::ends;
  return prompt.str();
}
}  // namespace

int main(int argc, char **argv) {
  struct stat stbuf;

  Sql db{};
  std::println("      ***   Galactic Bloodshed ver {0} ***", VERS);
  std::println();
  time_t clk = time(nullptr);
  std::print("      {0}", ctime(&clk));
#ifdef EXTERNAL_TRIGGER
  std::println("      The update  password is '%s'.", UPDATE_PASSWORD);
  std::println("      The segment password is '%s'.", SEGMENT_PASSWORD);
#endif
  int port;
  switch (argc) {
    case 2:
      port = std::stoi(argv[1]);
      update_time = DEFAULT_UPDATE_TIME;
      segments = MOVES_PER_UPDATE;
      break;
    case 3:
      port = std::stoi(argv[1]);
      update_time = std::stoi(argv[2]);
      segments = MOVES_PER_UPDATE;
      break;
    case 4:
      port = std::stoi(argv[1]);
      update_time = std::stoi(argv[2]);
      segments = std::stoi(argv[3]);
      break;
    default:
      port = GB_PORT;
      update_time = DEFAULT_UPDATE_TIME;
      segments = MOVES_PER_UPDATE;
      break;
  }
  std::cerr << "      Port " << port << std::endl;
  std::cerr << "      " << update_time << " minutes between updates"
            << std::endl;
  std::cerr << "      " << segments << " segments/update" << std::endl;
  start_buf = std::format("Server started  : {0}", ctime(&clk));

  next_update_time = clk + (update_time * 60);
  if (stat(UPDATEFL, &stbuf) >= 0) {
    if (FILE *sfile = fopen(UPDATEFL, "r"); sfile != nullptr) {
      char dum[32];
      if (fgets(dum, sizeof dum, sfile)) nupdates_done = atoi(dum);
      if (fgets(dum, sizeof dum, sfile)) last_update_time = atol(dum);
      if (fgets(dum, sizeof dum, sfile)) next_update_time = atol(dum);
      fclose(sfile);
    }
    update_buf = std::format("Last Update {:3d} : {}", nupdates_done,
                             ctime(&last_update_time));
  }
  if (segments <= 1)
    next_segment_time += (144 * 3600);
  else {
    next_segment_time = clk + (update_time * 60 / segments);
    if (stat(SEGMENTFL, &stbuf) >= 0) {
      if (FILE *sfile = fopen(SEGMENTFL, "r"); sfile != nullptr) {
        char dum[32];
        if (fgets(dum, sizeof dum, sfile)) nsegments_done = atoi(dum);
        if (fgets(dum, sizeof dum, sfile)) last_segment_time = atol(dum);
        if (fgets(dum, sizeof dum, sfile)) next_segment_time = atol(dum);
        fclose(sfile);
      }
    }
    if (next_segment_time < clk) { /* gvc */
      next_segment_time = next_update_time;
      nsegments_done = segments;
    }
  }
  segment_buf = std::format("Last Segment %{0:2d} : {1}", nsegments_done,
                            ctime(&last_segment_time));

  std::print(stderr, "{}", update_buf);
  std::print(stderr, "{}", segment_buf);
  srandom(getpid());
  std::print(stderr, "      Next Update {0}  : {1}", nupdates_done + 1,
             ctime(&next_update_time));
  std::print(stderr, "      Next Segment   : {0}", ctime(&next_segment_time));

  load_race_data(db); /* make sure you do this first */
  load_star_data();   /* get star data */
  getpower(Power);    /* get power report from database */
  Getblock(Blocks);   /* get alliance block data */
  SortShips();        /* Sort the ship list by tech for "build ?" */
  for (player_t i = 1; i <= MAXPLAYERS; i++) {
    setbit(Blocks[i - 1].invite, i - 1);
    setbit(Blocks[i - 1].pledge, i - 1);
  }
  Putblock(Blocks);
  compute_power_blocks();
  set_signals();
  int sock = shovechars(port, db);
  close_sockets(sock);
  std::println("Going down.");
  return 0;
}

static void set_signals() { signal(SIGPIPE, SIG_IGN); }

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

static int shovechars(int port, Db &db) {
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

  if (!shutdown_flag) post("Server started\n", NewsType::ANNOUNCE);

  newslength[NewsType::DECLARATION] = getnewslength(NewsType::DECLARATION);
  newslength[NewsType::TRANSFER] = getnewslength(NewsType::TRANSFER);
  newslength[NewsType::COMBAT] = getnewslength(NewsType::COMBAT);
  newslength[NewsType::ANNOUNCE] = getnewslength(NewsType::ANNOUNCE);

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
    for (auto &d : descriptor_list) {
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
          descriptor_list.emplace_back(sock, db);
          auto &newd = descriptor_list.back();
          make_nonblocking(newd.descriptor);
          welcome_user(newd);
        } catch (const std::runtime_error &) {
          perror("new_connection");
          return sock;
        }
      }

      for (auto &d : descriptor_list) {
        if (FD_ISSET(d.descriptor, &input_set)) {
          /*      d->last_time = now; */
          if (!process_input(d)) {
            shutdownsock(d);
            continue;
          }
        }
        if (FD_ISSET(d.descriptor, &output_set)) {
          if (!process_output(d)) {
            shutdownsock(d);
          }
        }
      }
    }
    if (go_time == 0) {
      if (now >= next_update_time) {
        go_time = now + (int_rand(0, DEFAULT_RANDOM_UPDATE_RANGE) * 60);
      }
      if (now >= next_segment_time && nsegments_done < segments) {
        go_time = now + (int_rand(0, DEFAULT_RANDOM_SEGMENT_RANGE) * 60);
      }
    }
    if (go_time > 0 && now >= go_time) {
      do_next_thing(db);
      go_time = 0;
    }
  }
  return sock;
}

void do_next_thing(Db &db) {
  if (nsegments_done < segments)
    do_segment(db, 0, 1);
  else
    do_update(db);
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
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(1);
  }
  if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, /* GVC */
                 (char *)&opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(1);
  }
  memset(&server, 0, sizeof(server));
  server.sin6_family = AF_INET6;
  server.sin6_addr = in6addr_any;
  server.sin6_port = htons(port);
  if (bind(s, (struct sockaddr *)&server, sizeof(server))) {
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
    for (auto &d : descriptor_list) {
      d.quota += COMMANDS_PER_TIME * nslices;
      if (d.quota > COMMAND_BURST_SIZE) d.quota = COMMAND_BURST_SIZE;
    }
  }
  return msec_add(last, nslices * COMMAND_TIME_MSEC);
}

static void shutdownsock(DescriptorData &d) {
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

static bool process_output(DescriptorData &d) {
  // Flush the stringstream buffer into the output queue.
  strstr_to_queue(d);

  while (!d.output.empty()) {
    auto &cur = d.output.front();
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
  for (auto &d : descriptor_list)
    if (d.connected) (void)process_output(d);
}

static void make_nonblocking(int s) {
  if (fcntl(s, F_SETFL, O_NDELAY) == -1) {
    perror("make_nonblocking: fcntl");
    exit(0);
  }
}

static void welcome_user(DescriptorData &d) {
  FILE *f;
  char *p;

  sprintf(buf,
          "***   Welcome to Galactic Bloodshed %s ***\nPlease enter your "
          "password:\n",
          VERS);
  queue_string(d, buf);

  if ((f = fopen(WELCOME_FILE, "r")) != nullptr) {
    while (fgets(buf, sizeof buf, f)) {
      for (p = buf; *p; p++)
        if (*p == '\n') {
          *p = '\0';
          break;
        }
      queue_string(d, buf);
      queue_string(d, "\n");
    }
    fclose(f);
  }
}

static void help_user(GameObj &g) {
  FILE *f;
  char *p;

  if ((f = fopen(HELP_FILE, "r")) != nullptr) {
    while (fgets(buf, sizeof buf, f)) {
      for (p = buf; *p; p++)
        if (*p == '\n') {
          *p = '\0';
          break;
        }
      notify(g.player, g.governor, buf);
      notify(g.player, g.governor, "\n");
    }
    fclose(f);
  }
}

static void goodbye_user(DescriptorData &d) {
  if (d.connected) /* this can happen, especially after updates */
    if (write(d.descriptor, LEAVE_MESSAGE, strlen(LEAVE_MESSAGE)) < 0) {
      perror("write error");
      exit(-1);
    }
}

static void save_command(DescriptorData &d, const std::string &command) {
  add_to_queue(d.input, command);
}

static int process_input(DescriptorData &d) {
  int got;
  char *p;
  char *pend;
  char *q;
  char *qend;

  got = read(d.descriptor, buf, sizeof buf);
  if (got <= 0) return 0;
  if (!d.raw_input) {
    d.raw_input = (char *)malloc(MAX_COMMAND_LEN * sizeof(char));
    d.raw_input_at = d.raw_input;
  }
  p = d.raw_input_at;
  pend = d.raw_input + MAX_COMMAND_LEN - 1;
  for (q = buf, qend = buf + got; q < qend; q++) {
    if (*q == '\n') {
      *p = '\0';
      if (p > d.raw_input) save_command(d, d.raw_input);
      p = d.raw_input;
    } else if (p < pend && isascii(*q) && isprint(*q)) {
      *p++ = *q;
    }
  }
  if (p > d.raw_input) {
    d.raw_input_at = p;
  } else {
    free(d.raw_input);
    d.raw_input = nullptr;
    d.raw_input_at = nullptr;
  }
  return 1;
}

static void process_commands() {
  int nprocessed;
  time_t now;

  (void)time(&now);

  do {
    nprocessed = 0;
    for (auto &d : descriptor_list) {
      if (d.quota > 0 && !d.input.empty()) {
        auto &t = d.input.front();
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
static bool do_command(DescriptorData &d, std::string_view comm) {
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
    sprintf(buf, "Emulating %s \"%s\" [%d,%d]\n", races[d.player - 1].name,
            races[d.player - 1].governor[d.governor].name, d.player,
            d.governor);
    queue_string(d, buf);
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

static void check_connect(DescriptorData &d, std::string_view message) {
  auto [race_password, gov_password] = parse_connect(message);

#ifdef EXTERNAL_TRIGGER
  if (race_password == SEGMENT_PASSWORD) {
    do_segment(d.db, 1, 0);
    return;
  } else if (race_password == UPDATE_PASSWORD) {
    do_update(d.db, true);
    return;
  }
#endif

  auto [Playernum, Governor] = getracenum(race_password, gov_password);

  if (!Playernum) {
    queue_string(d, "Connection refused.\n");
    std::println(stderr, "FAILED CONNECT {0},{1} on descriptor {2}\n",
                 race_password.c_str(), gov_password.c_str(), d.descriptor);
    return;
  }

  auto &race = races[Playernum - 1];
  /* check to see if this player is already connected, if so, nuke the
   * descriptor */
  for (auto &d0 : descriptor_list) {
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

  sprintf(buf, "\n%s \"%s\" [%d,%d] logged on.\n", race.name,
          race.governor[Governor].name, Playernum, Governor);
  notify_race(Playernum, buf);
  sprintf(buf, "You are %s.\n",
          race.governor[Governor].toggle.invisible ? "invisible" : "visible");
  notify(Playernum, Governor, buf);

  GB_time({}, d);
  sprintf(buf, "\nLast login      : %s",
          ctime(&(race.governor[Governor].login)));
  notify(Playernum, Governor, buf);
  race.governor[Governor].login = time(nullptr);
  putrace(race);
  if (!race.Gov_ship) {
    sprintf(buf,
            "You have no Governmental Center.  No action points will be "
            "produced\nuntil you build one and designate a capital.\n");
    notify(Playernum, Governor, buf);
  } else {
    sprintf(buf, "Government Center #%lu is active.\n", race.Gov_ship);
    notify(Playernum, Governor, buf);
  }
  sprintf(buf, "     Morale: %ld\n", race.morale);
  notify(Playernum, Governor, buf);
  GB::commands::treasury({}, d);
}

static void do_update(Db &db, bool force) {
  time_t clk = time(nullptr);
  int i;
  struct stat stbuf;
  int fakeit;

  fakeit = (!force && stat(NOGOFL, &stbuf) >= 0);

  sprintf(buf, "%sDOING UPDATE...\n", ctime(&clk));
  if (!fakeit) {
    for (i = 1; i <= Num_races; i++) notify_race(i, buf);
    force_output();
  }

  if (segments <= 1) {
    /* Disables movement segments. */
    next_segment_time = clk + (144 * 3600);
    nsegments_done = segments;
  } else {
    if (force)
      next_segment_time = clk + update_time * 60 / segments;
    else
      next_segment_time = next_update_time + update_time * 60 / segments;
    nsegments_done = 1;
  }
  if (force)
    next_update_time = clk + update_time * 60;
  else
    next_update_time += update_time * 60;

  if (!fakeit) nupdates_done++;

  Power_blocks.time = clk;
  update_buf =
      std::format("Last Update {0:3d} : {1}", nupdates_done, ctime(&clk));
  std::print(stderr, "{}", ctime(&clk));
  std::print(stderr, "Next Update {0:3d} : {1}", nupdates_done + 1,
             ctime(&next_update_time));
  segment_buf =
      std::format("Last Segment {0:2d} : {1}", nsegments_done, ctime(&clk));
  std::print(stderr, "{}", ctime(&clk));
  std::print(stderr, "Next Segment {0:2d} : {1}",
             nsegments_done == segments ? 1 : nsegments_done + 1,
             ctime(&next_segment_time));
  unlink(UPDATEFL);
  if (FILE *sfile = fopen(UPDATEFL, "w"); sfile != nullptr) {
    std::println(sfile, "{0}", nupdates_done);
    std::println(sfile, "{0}", clk);
    std::println(sfile, "{0}", next_update_time);
    fclose(sfile);
  }
  unlink(SEGMENTFL);
  if (FILE *sfile = fopen(SEGMENTFL, "w"); sfile != nullptr) {
    std::println(sfile, "{0}", nsegments_done);
    std::println(sfile, "{0}", clk);
    std::println(sfile, "{0}", next_segment_time);
    fflush(sfile);
    fclose(sfile);
  }

  update_flag = true;
  if (!fakeit) do_turn(db, 1);
  update_flag = false;
  clk = time(nullptr);
  sprintf(buf, "%sUpdate %d finished\n", ctime(&clk), nupdates_done);
  handle_victory();
  if (!fakeit) {
    for (i = 1; i <= Num_races; i++) notify_race(i, buf);
    force_output();
  }
}

static void do_segment(Db &db, int override, int segment) {
  time_t clk = time(nullptr);
  int i;
  struct stat stbuf;
  int fakeit;

  fakeit = (!override && stat(NOGOFL, &stbuf) >= 0);

  if (!override && segments <= 1) return;

  sprintf(buf, "%sDOING MOVEMENT...\n", ctime(&clk));
  if (!fakeit) {
    for (i = 1; i <= Num_races; i++) notify_race(i, buf);
    force_output();
  }
  if (override) {
    next_segment_time = clk + update_time * 60 / segments;
    if (segment) {
      nsegments_done = segment;
      next_update_time =
          clk + update_time * 60 * (segments - segment + 1) / segments;
    } else {
      nsegments_done++;
    }
  } else {
    next_segment_time += update_time * 60 / segments;
    nsegments_done++;
  }

  update_flag = true;
  if (!fakeit) do_turn(db, 0);
  update_flag = false;
  unlink(SEGMENTFL);
  if (FILE *sfile = fopen(SEGMENTFL, "w"); sfile != nullptr) {
    fprintf(sfile, "%d\n", nsegments_done);
    fprintf(sfile, "%ld\n", clk);
    fprintf(sfile, "%ld\n", next_segment_time);
    fclose(sfile);
  }
  segment_buf =
      std::format("Last Segment {0:2d} : {1}", nsegments_done, ctime(&clk));
  std::print(stderr, "{0}", ctime(&clk));
  std::print(stderr, "Next Segment {0:2d} : {1}", nsegments_done,
             ctime(&next_segment_time));
  clk = time(nullptr);
  sprintf(buf, "%sSegment finished\n", ctime(&clk));
  if (!fakeit) {
    for (i = 1; i <= Num_races; i++) notify_race(i, buf);
    force_output();
  }
}

static void close_sockets(int sock) {
  /* post message into news file */
  const char *shutdown_message = "Shutdown ordered by deity - Bye\n";
  post(shutdown_message, NewsType::ANNOUNCE);

  for (auto &d : descriptor_list) {
    if (write(d.descriptor, shutdown_message, strlen(shutdown_message)) < 0) {
      perror("write error");
      exit(-1);
    }
    if (shutdown(d.descriptor, 2) < 0) perror("shutdown");
    close(d.descriptor);
  }
  close(sock);
}

static void dump_users(DescriptorData &e) {
  time_t now;
  int God = 0;

  (void)time(&now);
  sprintf(buf, "Current Players: %s", ctime(&now));
  queue_string(e, buf);
  if (e.player) {
    auto &r = races[e.player - 1];
    God = r.God;
  } else
    return;

  for (auto &d : descriptor_list) {
    if (d.connected && !d.god) {
      auto &r = races[d.player - 1];
      if (!r.governor[d.governor].toggle.invisible || e.player == d.player ||
          God) {
        std::string temp = std::format("\"{}\"", r.governor[d.governor].name);
        sprintf(buf, "%20.20s %20.20s [%2d,%2d] %4lds idle %-4.4s %s %s\n",
                r.name, temp.c_str(), d.player, d.governor, now - d.last_time,
                God ? stars[d.snum].name : "    ",
                (r.governor[d.governor].toggle.gag ? "GAG" : "   "),
                (r.governor[d.governor].toggle.invisible ? "INVISIBLE" : ""));
        queue_string(e, buf);
      }

      if ((now - d.last_time) > DISCONNECT_TIME) d.connected = false;
    }
  }
#ifdef SHOW_COWARDS
  sprintf(buf, "And %d coward%s.\n", coward_count,
          (coward_count == 1) ? "" : "s");
  queue_string(e, buf);
#endif
  queue_string(e, "Finished.\n");
}

//* Dispatch to the function to run the command based on the string input by the
// user.
static void process_command(GameObj &g, const command_t &argv) {
  bool God = races[g.player - 1].God;

  auto command = commands.find(argv[0]);
  if (command != commands.end()) {
    command->second(argv, g);
  } else if (argv[0] == "purge" && God)
    purge();
  else if (argv[0] == "@@shutdown" && God) {
    shutdown_flag = 1;
    g.out << "Doing shutdown.\n";
  } else if (argv[0] == "@@update" && God)
    do_update(g.db, true);
  else if (argv[0] == "@@segment" && God)
    do_segment(g.db, 1, std::stoi(argv[1]));
  else {
    g.out << "'" << argv[0] << "':illegal command error.\n";
  }

  /* compute the prompt and send to the player */
  g.out << do_prompt(g);
}

static void load_race_data(Db &db) {
  Num_races = db.Numraces();
  races.reserve(Num_races);
  for (int i = 1; i <= Num_races; i++) {
    Race r = getrace(i); /* allocates into memory */
    if (r.Playernum != i) {
      r.Playernum = i;
      putrace(r);
    }
    races.push_back(r);
  }
}

/**
 * get star database
 */
static void load_star_data() {
  Planet_count = 0;
  getsdata(&Sdata);

  stars.reserve(Sdata.numstars);
  for (auto i = 0; i < Sdata.numstars; i++) {
    auto s = getstar(i);
    stars.push_back(s);
  }

  // TODO(jeffbailey): Convert this to be a range-based for loop.
  for (int s = 0; s < Sdata.numstars; s++) {
    for (int t = 0; t < stars[s].numplanets; t++) {
      planets[s][t] = std::make_unique<Planet>(getplanet(s, t));
      if (planets[s][t]->type != PlanetType::ASTEROID) Planet_count++;
    }
  }
}

/* report back the update status */
static void GB_time(const command_t &, GameObj &g) {
  time_t clk = time(nullptr);
  g.out << start_buf;
  g.out << update_buf;
  g.out << segment_buf;
  g.out << std::format("Current time    : {0}", ctime(&clk));
}

static void GB_schedule(const command_t &, GameObj &g) {
  time_t clk = time(nullptr);
  g.out << std::format("{0} minute update intervals\n", update_time);
  g.out << std::format("{0} movement segments per update\n", segments);
  g.out << std::format("Current time    : {0}", ctime(&clk));
  g.out << std::format("Next Segment {0:2d} : {1}",
                       nsegments_done == segments ? 1 : nsegments_done + 1,
                       ctime(&next_segment_time));
  g.out << std::format("Next Update {0:3d} : {1}", nupdates_done + 1,
                       ctime(&next_update_time));
}

static void help(const command_t &argv, GameObj &g) {
  FILE *f;
  char file[1024];
  char *p;

  if (argv.size() == 1) {
    help_user(g);
  } else {
    sprintf(file, "%s/%s.doc", DOCDIR, argv[1].c_str());
    if ((f = fopen(file, "r")) != nullptr) {
      while (fgets(buf, sizeof buf, f)) {
        for (p = buf; *p; p++)
          if (*p == '\n') {
            *p = '\0';
            break;
          }
        strcat(buf, "\n");
        notify(g.player, g.governor, buf);
      }
      fclose(f);
      notify(g.player, g.governor, "----\nFinished.\n");
    } else
      notify(g.player, g.governor, "Help on that subject unavailable.\n");
  }
}

void compute_power_blocks() {
  /* compute alliance block power */
  Power_blocks.time = time(nullptr);
  for (auto &i : races) {
    uint64_t dummy =
        Blocks[i.Playernum - 1].invite & Blocks[i.Playernum - 1].pledge;
    Power_blocks.members[i.Playernum - 1] = 0;
    Power_blocks.sectors_owned[i.Playernum - 1] = 0;
    Power_blocks.popn[i.Playernum - 1] = 0;
    Power_blocks.ships_owned[i.Playernum - 1] = 0;
    Power_blocks.resource[i.Playernum - 1] = 0;
    Power_blocks.fuel[i.Playernum - 1] = 0;
    Power_blocks.destruct[i.Playernum - 1] = 0;
    Power_blocks.money[i.Playernum - 1] = 0;
    Power_blocks.systems_owned[i.Playernum - 1] =
        Blocks[i.Playernum - 1].systems_owned;
    Power_blocks.VPs[i.Playernum - 1] = Blocks[i.Playernum - 1].VPs;
    for (auto &j : races) {
      if (isset(dummy, j.Playernum)) {
        Power_blocks.members[i.Playernum - 1] += 1;
        Power_blocks.sectors_owned[i.Playernum - 1] +=
            Power[j.Playernum - 1].sectors_owned;
        Power_blocks.money[i.Playernum - 1] += Power[j.Playernum - 1].money;
        Power_blocks.popn[i.Playernum - 1] += Power[j.Playernum - 1].popn;
        Power_blocks.ships_owned[i.Playernum - 1] +=
            Power[j.Playernum - 1].ships_owned;
        Power_blocks.resource[i.Playernum - 1] +=
            Power[j.Playernum - 1].resource;
        Power_blocks.fuel[i.Playernum - 1] += Power[j.Playernum - 1].fuel;
        Power_blocks.destruct[i.Playernum - 1] +=
            Power[j.Playernum - 1].destruct;
      }
    }
  }
}

/*utilities for dealing with ship lists */
void insert_sh_univ(stardata *sdata, Ship *s) {
  s->nextship = sdata->ships;
  sdata->ships = s->number;
  s->whatorbits = ScopeLevel::LEVEL_UNIV;
}

void insert_sh_star(Star &star, Ship *s) {
  s->nextship = star.ships;
  star.ships = s->number;
  s->whatorbits = ScopeLevel::LEVEL_STAR;
}

void insert_sh_plan(Planet &pl, Ship *s) {
  s->nextship = pl.ships;
  pl.ships = s->number;
  s->whatorbits = ScopeLevel::LEVEL_PLAN;
}

void insert_sh_ship(Ship *s, Ship *s2) {
  s->nextship = s2->ships;
  s2->ships = s->number;
  s->whatorbits = ScopeLevel::LEVEL_SHIP;
  s->whatdest = ScopeLevel::LEVEL_SHIP;
  s->destshipno = s2->number;
}

/**
 * \brief Remove a ship from the list of ships orbiting the star
 * \arg s Ship to remove
 */
void remove_sh_star(Ship &s) {
  stars[s.storbits] = getstar(s.storbits);
  shipnum_t sh = stars[s.storbits].ships;

  // If the ship is the first of the chain, point the star to the
  // next, which is zero if there are no other ships.
  if (sh == s.number) {
    stars[s.storbits].ships = s.nextship;
    putstar(stars[s.storbits], s.storbits);
  } else {
    Shiplist shiplist(sh);
    for (auto s2 : shiplist) {
      if (s2.nextship == s.number) {
        s2.nextship = s.nextship;
        putship(&s2);
        break;
      }
    }
  }

  // put in limbo - wait for insert_sh
  s.whatorbits = ScopeLevel::LEVEL_UNIV;
  s.nextship = 0;
}

/**
 * \brief Remove a ship from the list of ships orbiting the planet
 * \arg s Ship to remove
 */
void remove_sh_plan(Ship &s) {
  auto host = getplanet(s.storbits, s.pnumorbits);
  shipnum_t sh = host.ships;

  // If the ship is the first of the chain, point the star to the
  // next, which is zero if there are no other ships.
  if (sh == s.number) {
    host.ships = s.nextship;
    putplanet(host, stars[s.storbits], s.pnumorbits);
  } else {
    Shiplist shiplist(sh);
    for (auto s2 : shiplist) {
      if (s2.nextship == s.number) {
        s2.nextship = s.nextship;
        putship(&s2);
        break;
      }
    }
  }

  // put in limbo - wait for insert_sh
  s.whatorbits = ScopeLevel::LEVEL_UNIV;
  s.nextship = 0;
}

/**
 * \brief Remove a ship from the list of ships in the ship
 * \arg s Ship to remove
 */
void remove_sh_ship(Ship &s, Ship &host) {
  shipnum_t sh = host.ships;

  // If the ship is the first of the chain, point the ship to the
  // next, which is zero if there are no other ships.
  if (sh == s.number) {
    host.ships = s.nextship;
  } else {
    Shiplist shiplist(sh);
    for (auto s2 : shiplist) {
      if (s2.nextship == s.number) {
        s2.nextship = s.nextship;
        putship(&s2);
        break;
      }
    }
  }

  // put in limbo - wait for insert_sh
  s.whatorbits = ScopeLevel::LEVEL_UNIV;
  s.nextship = 0;
}

static double GetComplexity(const ShipType ship) {
  Ship s;

  s.armor = Shipdata[ship][ABIL_ARMOR];
  s.guns = Shipdata[ship][ABIL_PRIMARY] ? PRIMARY : GTYPE_NONE;
  s.primary = Shipdata[ship][ABIL_GUNS];
  s.primtype = Shipdata[ship][ABIL_PRIMARY];
  s.secondary = Shipdata[ship][ABIL_GUNS];
  s.sectype = Shipdata[ship][ABIL_SECONDARY] ? SECONDARY : GTYPE_NONE;
  s.max_crew = Shipdata[ship][ABIL_MAXCREW];
  s.max_resource = Shipdata[ship][ABIL_CARGO];
  s.max_hanger = Shipdata[ship][ABIL_HANGER];
  s.max_destruct = Shipdata[ship][ABIL_DESTCAP];
  s.max_fuel = Shipdata[ship][ABIL_FUELCAP];
  s.max_speed = Shipdata[ship][ABIL_SPEED];
  s.build_type = ship;
  s.mount = Shipdata[ship][ABIL_MOUNT];
  s.hyper_drive.has = Shipdata[ship][ABIL_JUMP];
  s.cloak = 0;
  s.laser = Shipdata[ship][ABIL_LASER];
  s.cew = 0;
  s.cew_range = 0;
  s.size = ship_size(s);
  s.base_mass = getmass(s);
  s.mass = getmass(s);

  return complexity(s);
}

static int ShipCompare(const void *S1, const void *S2) {
  const auto *s1 = (const ShipType *)S1;
  const auto *s2 = (const ShipType *)S2;
  return (int)(GetComplexity(*s1) - GetComplexity(*s2));
}

static void SortShips() {
  for (int i = 0; i < NUMSTYPES; i++) ShipVector[i] = i;
  qsort(ShipVector, NUMSTYPES, sizeof(int), ShipCompare);
}
