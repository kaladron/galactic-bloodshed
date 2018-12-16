// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "GB_server.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "analysis.h"
#include "autoreport.h"
#include "buffers.h"
#include "build.h"
#include "capital.h"
#include "capture.h"
#include "config.h"
#include "cs.h"
#include "declare.h"
#include "defense.h"
#include "dissolve.h"
#include "dock.h"
#include "doturncmd.h"
#include "enslave.h"
#include "examine.h"
#include "explore.h"
#include "files.h"
#include "files_shl.h"
#include "fire.h"
#include "fuel.h"
#include "land.h"
#include "launch.h"
#include "load.h"
#include "map.h"
#include "mobiliz.h"
#include "move.h"
#include "name.h"
#include "orbit.h"
#include "order.h"
#include "power.h"
#include "powercmd.h"
#include "prof.h"
#include "races.h"
#include "rand.h"
#include "relation.h"
#include "rst.h"
#include "scrap.h"
#include "ships.h"
#include "shlmisc.h"
#include "survey.h"
#include "tech.h"
#include "tele.h"
#include "toggle.h"
#include "toxicity.h"
#include "tweakables.h"
#include "vars.h"
#include "victory.h"
#include "zoom.h"

#include "globals.h"

static struct stat sbuf;

static int shutdown_flag = 0;
static int update_flag = 0;

static time_t last_update_time;
static time_t last_segment_time;
static unsigned int nupdates_done; /* number of updates so far */

static char start_buf[128];
static char update_buf[128];
static char segment_buf[128];

static const char *connect_fail = "Connection refused.\n";
static const char *shutdown_message = "Shutdown ordered by deity - Bye\n";
static const char *already_on = "Connection refused.\n";

class TextBlock {
 public:
  TextBlock(const std::string &in) {
    nchars = in.size();
    buf = strdup(in.c_str());
    start = buf;
  }

  ~TextBlock() { free(buf); }
  int nchars;
  char *start;
  char *buf;
  TextBlock(const TextBlock &) = delete;
  TextBlock &operator=(const TextBlock &) = delete;
};

class DescriptorData : public GameObj {
 public:
  DescriptorData(int s)
      : descriptor(s),
        connected(false),
        output_size(0),
        raw_input(nullptr),
        raw_input_at(nullptr),
        last_time(0),
        quota(COMMAND_BURST_SIZE) {}
  const int descriptor;
  bool connected;
  ssize_t output_size;
  std::deque<TextBlock> output;
  std::deque<TextBlock> input;
  char *raw_input;
  char *raw_input_at;
  time_t last_time;
  int quota;
};

static double GetComplexity(int);
static void set_signals(void);
static void queue_string(DescriptorData *, const std::string &);
static void add_to_queue(std::deque<TextBlock> &, const std::string &);
static void help(const command_t &, GameObj &);
static void process_command(DescriptorData &, const command_t &argv);
static int shovechars(int);

static void GB_time(const command_t &, GameObj &);
static void GB_schedule(const command_t &, GameObj &);
static DescriptorData *new_connection(int);
static void do_update(int);
static void do_segment(int, int);
static int make_socket(int);
static void shutdownsock(DescriptorData *);
static void load_race_data(void);
static void load_star_data(void);
static void make_nonblocking(int);
static struct timeval update_quotas(struct timeval, struct timeval);
static int process_output(DescriptorData *);
static void welcome_user(DescriptorData *);
static int flush_queue(std::deque<TextBlock> &, int);
static void process_commands(void);
static int do_command(DescriptorData *, const char *);
static void goodbye_user(DescriptorData *);
static void dump_users(DescriptorData *);
static void close_sockets(int);
static int process_input(DescriptorData *);
static void force_output(void);
static void help_user(GameObj &);
static void parse_connect(const char *, char *, char *);
static int msec_diff(struct timeval, struct timeval);
static struct timeval msec_add(struct timeval, int);
static void save_command(DescriptorData *, const std::string &);
static std::string do_prompt(DescriptorData &);
static void strstr_to_queue(DescriptorData *);

static void check_connect(DescriptorData *, const char *);
static struct timeval timeval_sub(struct timeval now, struct timeval then);

#define MAX_COMMAND_LEN 512

static std::list<DescriptorData *> descriptor_list;

typedef void (*CommandFunction)(const command_t &, GameObj &);

static const std::unordered_map<std::string, CommandFunction> commands{
    {"'", announce},
    {"allocate", allocateAPs},
    {"analysis", analysis},
    {"announce", announce},
    {"appoint", governors},
    {"assault", dock},
    {"arm", arm},
    {"autoreport", autoreport},
#ifdef MARKET
    {"bid", bid},
#endif
    {"bless", bless},
    {"block", block},
    {"bombard", bombard},  // TODO(jeffbailey): !guest
    {"broadcast", announce},
    {"build", build},
    {"capital", capital},
    {"capture", capture},
    {"center", center},
    {"cew", fire},
    {"client_survey", survey},
    {"colonies", colonies},
    {"cs", cs},
    {"declare", declare},
#ifdef DEFENSE
    {"defend", defend},
#endif
    {"deploy", move_popn},
    {"detonate", detonate},  // TODO(jeffbailey): !guest
    {"disarm", arm},
    {"dismount", mount},
    {"dissolve", dissolve},  // TODO(jeffbailey): !guest
    {"distance", distance},
    {"dock", dock},
    {"dump", dump},
    {"enslave", enslave},
    {"examine", examine},
    {"explore", exploration},
    {"factories", rst},
    {"fire", fire},  // TODO(jeffbailey): !guest
    {"fix", fix},
    {"fuel", proj_fuel},
    {"give", give},  // TODO(jeffbailey): !guest
    {"governors", governors},
    {"grant", grant},
    {"help", help},
    {"highlight", highlight},
    {"identify", whois},
#ifdef MARKET
    {"insurgency", insurgency},
#endif
    {"invite", invite},
    {"jettison", jettison},
    {"land", land},
    {"launch", launch},
    {"load", load},
    {"make", make_mod},
    {"map", map},
    {"mobilize", mobilize},
    {"modify", make_mod},
    {"move", move_popn},
    {"mount", mount},
    {"motto", motto},
    {"name", name},
    {"orbit", orbit},
    {"order", order},
    {"page", page},
    {"pay", pay},  // TODO(jeffbailey): !guest
    {"personal", personal},
    {"pledge", pledge},
    {"power", power},
    {"profile", profile},
    {"post", send_message},
    {"production", colonies},
    {"relation", relation},
    {"read", read_messages},
    {"repair", repair},
    {"report", rst},
    {"revoke", governors},
    {"route", route},
    {"schedule", GB_schedule},
    {"scrap", scrap},
#ifdef MARKET
    {"sell", sell},
#endif
    {"send", send_message},
    {"shout", announce},
    {"survey", survey},
    {"ship", rst},
    {"stars", star_locations},
    {"stats", rst},
    {"status", tech_status},
    {"stock", rst},
    {"tactical", rst},
    {"technology", technology},
    {"think", announce},
    {"time", GB_time},
#ifdef MARKET
    {"tax", tax},
#endif
    {"toggle", toggle},
    {"toxicity", toxicity},
    {"transfer", transfer},
#ifdef MARKET
    {"treasury", treasury},
#endif
    {"undock", launch},
    {"uninvite", invite},
    {"unload", load},
    {"unpledge", pledge},
    {"upgrade", upgrade},
    {"victory", victory},
#ifdef VOTING
    {"vote", vote},
#endif
    {"walk", walk},
    {"whois", whois},
    {"weapons", rst},
    {"zoom", zoom},
};

int main(int argc, char **argv) {
  int i;
  struct stat stbuf;
  FILE *sfile;

  open_data_files();
  printf("      ***   Galactic Bloodshed ver %s ***\n\n", VERS);
  time_t clk = time(nullptr);
  printf("      %s", ctime(&clk));
#ifdef EXTERNAL_TRIGGER
  printf("      The update  password is '%s'.\n", UPDATE_PASSWORD);
  printf("      The segment password is '%s'.\n", SEGMENT_PASSWORD);
#endif
  int port;
  switch (argc) {
    case 2:
      port = atoi(argv[1]);
      update_time = DEFAULT_UPDATE_TIME;
      segments = MOVES_PER_UPDATE;
      break;
    case 3:
      port = atoi(argv[1]);
      update_time = atoi(argv[2]);
      segments = MOVES_PER_UPDATE;
      break;
    case 4:
      port = atoi(argv[1]);
      update_time = atoi(argv[2]);
      segments = atoi(argv[3]);
      break;
    default:
      port = GB_PORT;
      update_time = DEFAULT_UPDATE_TIME;
      segments = MOVES_PER_UPDATE;
      break;
  }
  fprintf(stderr, "      Port %d\n", port);
  fprintf(stderr, "      %d minutes between updates\n", update_time);
  fprintf(stderr, "      %ld segments/update\n", segments);
  sprintf(start_buf, "Server started  : %s", ctime(&clk));

  next_update_time = clk + (update_time * 60);
  if (stat(UPDATEFL, &stbuf) >= 0) {
    if ((sfile = fopen(UPDATEFL, "r"))) {
      char dum[32];
      if (fgets(dum, sizeof dum, sfile)) nupdates_done = atoi(dum);
      if (fgets(dum, sizeof dum, sfile)) last_update_time = atol(dum);
      if (fgets(dum, sizeof dum, sfile)) next_update_time = atol(dum);
      fclose(sfile);
    }
    sprintf(update_buf, "Last Update %3d : %s", nupdates_done,
            ctime(&last_update_time));
  }
  if (segments <= 1)
    next_segment_time += (144 * 3600);
  else {
    next_segment_time = clk + (update_time * 60 / segments);
    if (stat(SEGMENTFL, &stbuf) >= 0) {
      if ((sfile = fopen(SEGMENTFL, "r"))) {
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
  sprintf(segment_buf, "Last Segment %2d : %s", nsegments_done,
          ctime(&last_segment_time));

  fprintf(stderr, "%s", update_buf);
  fprintf(stderr, "%s", segment_buf);
  srandom(getpid());
  fprintf(stderr, "      Next Update %d  : %s", nupdates_done + 1,
          ctime(&next_update_time));
  fprintf(stderr, "      Next Segment   : %s", ctime(&next_segment_time));

  load_race_data(); /* make sure you do this first */
  load_star_data(); /* get star data */
  Getpower(Power);  /* get power report from disk */
  Getblock(Blocks); /* get alliance block data */
  SortShips();      /* Sort the ship list by tech for "build ?" */
  for (i = 1; i <= MAXPLAYERS; i++) {
    setbit(Blocks[i - 1].invite, i - 1);
    setbit(Blocks[i - 1].pledge, i - 1);
  }
  Putblock(Blocks);
  compute_power_blocks();
  set_signals();
  int sock = shovechars(port);
  close_sockets(sock);
  close_data_files();
  printf("Going down.\n");
  return (0);
}

static void set_signals(void) { signal(SIGPIPE, SIG_IGN); }

void notify_race(player_t race, const std::string &message) {
  if (update_flag) return;
  for (auto d : descriptor_list) {
    if (d->connected && d->player == race) {
      queue_string(d, message);
    }
  }
}

bool notify(player_t race, governor_t gov, const std::string &message) {
  if (update_flag) return 0;
  for (auto d : descriptor_list)
    if (d->connected && d->player == race && d->governor == gov) {
      strstr_to_queue(d);  // Ensuring anything queued up is flushed out.
      queue_string(d, message);
      return true;
    }
  return false;
}

void d_think(player_t Playernum, governor_t Governor,
             const std::string &message) {
  for (auto d : descriptor_list) {
    if (d->connected && d->player == Playernum && d->governor != Governor &&
        !races[d->player - 1]->governor[d->governor].toggle.gag) {
      queue_string(d, message);
    }
  }
}

void d_broadcast(player_t Playernum, governor_t Governor,
                 const std::string &message) {
  for (auto d : descriptor_list) {
    if (d->connected && !(d->player == Playernum && d->governor == Governor) &&
        !races[d->player - 1]->governor[d->governor].toggle.gag) {
      queue_string(d, message);
    }
  }
}

void d_shout(player_t Playernum, governor_t Governor,
             const std::string &message) {
  for (auto d : descriptor_list) {
    if (d->connected && !(d->player == Playernum && d->governor == Governor)) {
      queue_string(d, message);
    }
  }
}

void d_announce(player_t Playernum, governor_t Governor, starnum_t star,
                const std::string &message) {
  for (auto d : descriptor_list) {
    if (d->connected && !(d->player == Playernum && d->governor == Governor) &&
        (isset(Stars[star]->inhabited, d->player) ||
         races[d->player - 1]->God) &&
        d->snum == star &&
        !races[d->player - 1]->governor[d->governor].toggle.gag) {
      queue_string(d, message);
    }
  }
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

static int shovechars(int port) {  // __attribute__((no_sanitize_memory)) {
  fd_set input_set, output_set;
  struct timeval last_slice, current_time;
  struct timeval next_slice;
  struct timeval timeout, slice_timeout;
  time_t now;
  time_t go_time;

  int sock = make_socket(port);
  gettimeofday(&last_slice, nullptr);

  if (!shutdown_flag) post("Server started\n", ANNOUNCE);
  for (int i = 0; i <= ANNOUNCE; i++) newslength[i] = Newslength(i);

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
    for (auto d : descriptor_list) {
      if (!d->input.empty())
        timeout = slice_timeout;
      else
        FD_SET(d->descriptor, &input_set);
      // Is there anything in the output queue?
      if (!d->output.empty() || !d->out.str().empty())
        FD_SET(d->descriptor, &output_set);
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
        DescriptorData *newd;
        if (!(newd = new_connection(sock))) {
          if (errno && errno != EINTR && errno != EMFILE) {
            perror("new_connection");
            return sock;
          }
        }
        descriptor_list.push_back(newd);
        welcome_user(newd);
      }

      // TODO(jeffbailey): There's a use-after-free here if the connection is
      // closed without typing anything or completing the connection.
      for (auto d : descriptor_list) {
        if (FD_ISSET(d->descriptor, &input_set)) {
          /*      d->last_time = now; */
          if (!process_input(d)) {
            shutdownsock(d);
            continue;
          }
        }
        if (FD_ISSET(d->descriptor, &output_set)) {
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
      (void)do_next_thing();
      go_time = 0;
    }
  }
  return sock;
}

void do_next_thing(void) {
  if (nsegments_done < segments)
    do_segment(0, 1);
  else
    do_update(0);
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
  int nslices;

  nslices = msec_diff(current, last) / COMMAND_TIME_MSEC;

  if (nslices > 0) {
    for (auto d : descriptor_list) {
      d->quota += COMMANDS_PER_TIME * nslices;
      if (d->quota > COMMAND_BURST_SIZE) d->quota = COMMAND_BURST_SIZE;
    }
  }
  return msec_add(last, nslices * COMMAND_TIME_MSEC);
}

static DescriptorData *new_connection(int sock) {
  int newsock;
  struct sockaddr_in6 addr;
  socklen_t addr_len = sizeof(addr);
  char addrstr[INET6_ADDRSTRLEN];

  newsock = accept(sock, (struct sockaddr *)&addr, &addr_len);
  if (newsock <= 0) return nullptr;
  inet_ntop(AF_INET6, &addr.sin6_addr, addrstr, sizeof(addrstr));
  // TODO(jeffbailey): There used to be custom access check stuff here.
  // It should be replaced with TCP wrappers or something similar
  fprintf(stderr, "ACCEPT from %s(%d) on descriptor %d\n", addrstr,
          ntohs(addr.sin6_port), newsock);
  make_nonblocking(newsock);
  return new DescriptorData(newsock);
}

static void shutdownsock(DescriptorData *d) {
  if (d->connected) {
    fprintf(stderr, "DISCONNECT %d Race=%d Governor=%d\n", d->descriptor,
            d->player, d->governor);
  } else {
    fprintf(stderr, "DISCONNECT %d never connected\n", d->descriptor);
  }
  shutdown(d->descriptor, 2);
  close(d->descriptor);
  descriptor_list.remove(d);
  delete d;
}

static void add_to_queue(std::deque<TextBlock> &q, const std::string &b) {
  if (b.empty()) return;

  q.emplace_back(b);
}

static int flush_queue(std::deque<TextBlock> &q, int n) {
  int really_flushed = 0;

  const char *flushed_message = "<Output Flushed>\n";
  n += strlen(flushed_message);

  while (n > 0 && !q.empty()) {
    auto &p = q.front();
    n -= p.nchars;
    really_flushed += p.nchars;
    q.pop_front();
  }
  q.emplace_back(flushed_message);
  really_flushed -= strlen(flushed_message);
  return really_flushed;
}

static void queue_string(DescriptorData *d, const std::string &b) {
  if (b.empty()) return;
  int space = MAX_OUTPUT - d->output_size - b.size();
  if (space < 0) d->output_size -= flush_queue(d->output, -space);
  add_to_queue(d->output, b);
  d->output_size += b.size();
}

//* Push contents of the stream to the queues
static void strstr_to_queue(DescriptorData *d) {
  if (d->out.str().empty()) return;
  queue_string(d, d->out.str());
  d->out.clear();
  d->out.str("");
}

static int process_output(DescriptorData *d) {
  ssize_t cnt;

  // Flush the stringstream buffer into the output queue.
  strstr_to_queue(d);

  while (!d->output.empty()) {
    auto &cur = d->output.front();
    cnt = write(d->descriptor, cur.start, cur.nchars);
    if (cnt < 0) {
      if (errno == EWOULDBLOCK) return 1;
      d->connected = false;
      return 0;
    }
    d->output_size -= cnt;
    if (cnt == cur.nchars) {  // We output the entire block
      d->output.pop_front();
      continue;
    }
    // We only output part of it, so we don't clear it out.
    cur.nchars -= cnt;
    cur.start += cnt;
    break;
  }
  return 1;
}

static void force_output(void) {
  for (auto d : descriptor_list)
    if (d->connected) (void)process_output(d);
}

static void make_nonblocking(int s) {
  if (fcntl(s, F_SETFL, O_NDELAY) == -1) {
    perror("make_nonblocking: fcntl");
    exit(0);
  }
}

static void welcome_user(DescriptorData *d) {
  FILE *f;
  char *p;

  sprintf(buf, "***   Welcome to Galactic Bloodshed %s ***\n", VERS);
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

static void goodbye_user(DescriptorData *d) {
  if (d->connected) /* this can happen, especially after updates */
    write(d->descriptor, LEAVE_MESSAGE, strlen(LEAVE_MESSAGE));
}

static void save_command(DescriptorData *d, const std::string &command) {
  add_to_queue(d->input, command);
}

static int process_input(DescriptorData *d) {
  int got;
  char *p, *pend, *q, *qend;

  got = read(d->descriptor, buf, sizeof buf);
  if (got <= 0) return 0;
  if (!d->raw_input) {
    d->raw_input = (char *)malloc(MAX_COMMAND_LEN * sizeof(char));
    d->raw_input_at = d->raw_input;
  }
  p = d->raw_input_at;
  pend = d->raw_input + MAX_COMMAND_LEN - 1;
  for (q = buf, qend = buf + got; q < qend; q++) {
    if (*q == '\n') {
      *p = '\0';
      if (p > d->raw_input) save_command(d, d->raw_input);
      p = d->raw_input;
    } else if (p < pend && isascii(*q) && isprint(*q)) {
      *p++ = *q;
    }
  }
  if (p > d->raw_input) {
    d->raw_input_at = p;
  } else {
    free(d->raw_input);
    d->raw_input = nullptr;
    d->raw_input_at = nullptr;
  }
  return 1;
}

static void process_commands(void) {
  int nprocessed;
  long now;

  (void)time(&now);

  do {
    nprocessed = 0;
    for (auto d : descriptor_list) {
      if (d->quota > 0 && !d->input.empty()) {
        auto &t = d->input.front();
        d->quota--;
        nprocessed++;

        if (!do_command(d, t.start)) {
          shutdownsock(d);
          break;
        } else {
          d->last_time = now; /* experimental code */
          d->input.pop_front();
          d->last_time = now; /* experimental code */
        }
      }
    }
  } while (nprocessed > 0);
}

static int do_command(DescriptorData *d, const char *comm) {
  /* Main processing loop. When command strings are sent from the client,
     they are processed here. Responses are sent back to the client via
     notify.
     */

  /* check to see if there are a few words typed out, usually for the help
   * command */
  command_t argv;
  boost::split(argv, comm, boost::is_any_of(" "));

  if (argv[0] == "quit") {
    goodbye_user(d);
    return 0;
  } else if (d->connected && argv[0] == "who") {
    dump_users(d);
  } else if (d->connected && d->god && argv[0] == "emulate") {
    d->player = std::stoi(argv[1]);
    d->governor = std::stoi(argv[2]);
    sprintf(buf, "Emulating %s \"%s\" [%d,%d]\n", races[d->player - 1]->name,
            races[d->player - 1]->governor[d->governor].name, d->player,
            d->governor);
    queue_string(d, buf);
  } else {
    if (d->connected) {
      /* GB command parser */
      process_command(*d, argv);
    } else {
      check_connect(
          d, comm); /* Logs player into the game, connects
                          if the password given by *command is a player's */
      if (!d->connected) {
        goodbye_user(d);
      } else {
        check_for_telegrams(d->player, d->governor);
        /* set the scope to home upon login */
        command_t call_cs = {"cs"};
        process_command(*d, call_cs);
      }
    }
  }
  return 1;
}

static void check_connect(DescriptorData *d, const char *message) {
  char race_password[MAX_COMMAND_LEN];
  char gov_password[MAX_COMMAND_LEN];
  int i, j;
  int Playernum, Governor;
  racetype *r;

  parse_connect(message, race_password, gov_password);

#ifdef EXTERNAL_TRIGGER
  if (!strcmp(race_password, SEGMENT_PASSWORD)) {
    do_segment(1, 0);
    return;
  } else if (!strcmp(race_password, UPDATE_PASSWORD)) {
    do_update(1);
    return;
  }
#endif

  Getracenum(race_password, gov_password, &i, &j);

  if (!i) {
    queue_string(d, connect_fail);
    fprintf(stderr, "FAILED CONNECT %s,%s on descriptor %d\n", race_password,
            gov_password, d->descriptor);
  } else {
    Playernum = i;
    Governor = j;
    r = races[i - 1];
    /* check to see if this player is already connect, if so, nuke the
     * descriptor */
    for (auto d0 : descriptor_list) {
      if (d0->connected && d0->player == Playernum &&
          d0->governor == Governor) {
        queue_string(d, already_on);
        return;
      }
    }

    fprintf(stderr, "CONNECTED %s \"%s\" [%d,%d] on descriptor %d\n", r->name,
            r->governor[j].name, Playernum, Governor, d->descriptor);
    d->connected = true;

    d->god = r->God;
    d->player = Playernum;
    d->governor = Governor;

    sprintf(buf, "\n%s \"%s\" [%d,%d] logged on.\n", r->name,
            r->governor[j].name, Playernum, Governor);
    notify_race(Playernum, buf);
    sprintf(buf, "You are %s.\n",
            r->governor[j].toggle.invisible ? "invisible" : "visible");
    notify(Playernum, Governor, buf);

    GB_time({}, *d);
    sprintf(buf, "\nLast login      : %s", ctime(&(r->governor[j].login)));
    notify(Playernum, Governor, buf);
    r->governor[j].login = time(nullptr);
    putrace(r);
    if (!r->Gov_ship) {
      sprintf(buf,
              "You have no Governmental Center.  No action points will be "
              "produced\nuntil you build one and designate a capital.\n");
      notify(Playernum, Governor, buf);
    } else {
      sprintf(buf, "Government Center #%lu is active.\n", r->Gov_ship);
      notify(Playernum, Governor, buf);
    }
    sprintf(buf, "     Morale: %ld\n", r->morale);
    notify(Playernum, Governor, buf);
    treasury({}, *d);
  }
}

static void do_update(int override) {
  time_t clk = time(nullptr);
  int i;
  FILE *sfile;
  struct stat stbuf;
  int fakeit;

  fakeit = (!override && stat(NOGOFL, &stbuf) >= 0);

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
    if (override)
      next_segment_time = clk + update_time * 60 / segments;
    else
      next_segment_time = next_update_time + update_time * 60 / segments;
    nsegments_done = 1;
  }
  if (override)
    next_update_time = clk + update_time * 60;
  else
    next_update_time += update_time * 60;

  if (!fakeit) nupdates_done++;

  sprintf(Power_blocks.time, "%s", ctime(&clk));
  sprintf(update_buf, "Last Update %3d : %s", nupdates_done, ctime(&clk));
  fprintf(stderr, "%s", ctime(&clk));
  fprintf(stderr, "Next Update %3d : %s", nupdates_done + 1,
          ctime(&next_update_time));
  sprintf(segment_buf, "Last Segment %2d : %s", nsegments_done, ctime(&clk));
  fprintf(stderr, "%s", ctime(&clk));
  fprintf(stderr, "Next Segment %2d : %s",
          nsegments_done == segments ? 1 : nsegments_done + 1,
          ctime(&next_segment_time));
  unlink(UPDATEFL);
  if ((sfile = fopen(UPDATEFL, "w"))) {
    fprintf(sfile, "%d\n", nupdates_done);
    fprintf(sfile, "%ld\n", clk);
    fprintf(sfile, "%ld\n", next_update_time);
    fflush(sfile);
    fclose(sfile);
  }
  unlink(SEGMENTFL);
  if ((sfile = fopen(SEGMENTFL, "w"))) {
    fprintf(sfile, "%d\n", nsegments_done);
    fprintf(sfile, "%ld\n", clk);
    fprintf(sfile, "%ld\n", next_segment_time);
    fflush(sfile);
    fclose(sfile);
  }

  update_flag = 1;
  if (!fakeit) do_turn(1);
  update_flag = 0;
  clk = time(nullptr);
  sprintf(buf, "%sUpdate %d finished\n", ctime(&clk), nupdates_done);
  handle_victory();
  if (!fakeit) {
    for (i = 1; i <= Num_races; i++) notify_race(i, buf);
    force_output();
  }
}

static void do_segment(int override, int segment) {
  time_t clk = time(nullptr);
  int i;
  FILE *sfile;
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

  update_flag = 1;
  if (!fakeit) do_turn(0);
  update_flag = 0;
  unlink(SEGMENTFL);
  if ((sfile = fopen(SEGMENTFL, "w"))) {
    fprintf(sfile, "%d\n", nsegments_done);
    fprintf(sfile, "%ld\n", clk);
    fprintf(sfile, "%ld\n", next_segment_time);
    fflush(sfile);
    fclose(sfile);
  }
  sprintf(segment_buf, "Last Segment %2d : %s", nsegments_done, ctime(&clk));
  fprintf(stderr, "%s", ctime(&clk));
  fprintf(stderr, "Next Segment %2d : %s", nsegments_done,
          ctime(&next_segment_time));
  clk = time(nullptr);
  sprintf(buf, "%sSegment finished\n", ctime(&clk));
  if (!fakeit) {
    for (i = 1; i <= Num_races; i++) notify_race(i, buf);
    force_output();
  }
}

static void parse_connect(const char *message, char *race_pass,
                          char *gov_pass) {
  char *p;
  char *q;
  /* race password */
  while (*message && isascii(*message) && isspace(*message)) message++;
  p = race_pass;
  while (*message && isascii(*message) && !isspace(*message)) *p++ = *message++;
  *p = '\0';
  /* governor password */
  while (*message && isascii(*message) && isspace(*message)) message++;
  q = gov_pass;
  while (*message && isascii(*message) && !isspace(*message)) *q++ = *message++;
  *q = '\0';
}

static void close_sockets(int sock) {
  /* post message into news file */
  post(shutdown_message, ANNOUNCE);

  for (auto d : descriptor_list) {
    write(d->descriptor, shutdown_message, strlen(shutdown_message));
    if (shutdown(d->descriptor, 2) < 0) perror("shutdown");
    close(d->descriptor);
  }
  close(sock);
}

static void dump_users(DescriptorData *e) {
  long now;
  racetype *r;
  int God = 0;
  int coward_count = 0;

  (void)time(&now);
  sprintf(buf, "Current Players: %s", ctime(&now));
  queue_string(e, buf);
  if (e->player) {
    r = races[e->player - 1];
    God = r->God;
  } else
    return;

  for (auto d : descriptor_list) {
    if (d->connected && !d->god) {
      r = races[d->player - 1];
      if (!r->governor[d->governor].toggle.invisible ||
          e->player == d->player || God) {
        sprintf(temp, "\"%s\"", r->governor[d->governor].name);
        sprintf(buf, "%20.20s %20.20s [%2d,%2d] %4lds idle %-4.4s %s %s\n",
                r->name, temp, d->player, d->governor, now - d->last_time,
                God ? Stars[d->snum]->name : "    ",
                (r->governor[d->governor].toggle.gag ? "GAG" : "   "),
                (r->governor[d->governor].toggle.invisible ? "INVISIBLE" : ""));
        queue_string(e, buf);
      } else if (!God) /* deity lurks around */
        coward_count++;

      if ((now - d->last_time) > DISCONNECT_TIME) d->connected = 0;
    }
  }
#ifdef SHOW_COWARDS
  sprintf(buf, "And %d coward%s.\n", coward_count,
          (coward_count == 1) ? "" : "s");
  queue_string(e, buf);
#endif
  queue_string(e, "Finished.\n");
}

static void process_command(DescriptorData &d, const command_t &argv) {
  int God, Guest;
  racetype *r;

  int Playernum = d.player;
  int Governor = d.governor;

  /*determine which routine to go to and what action to take */
  /* target routine is specified by the first substring, other options
  are the substrings which follow */

  r = races[Playernum - 1];
  God = r->God;
  Guest = r->Guest;

  auto command = commands.find(argv[0]);
  if (command != commands.end()) {
    command->second(argv, d);
  } else if (match(argv[0].c_str(), "purge") && God)
    purge();
  else if (match(argv[0].c_str(), "@@shutdown") && God) {
    shutdown_flag = 1;
    notify(Playernum, Governor, "Doing shutdown.\n");
  } else if (match(argv[0].c_str(), "@@update") && God)
    do_update(1);
  else if (match(argv[0].c_str(), "@@segment") && God)
    do_segment(1, atoi(argv[1].c_str()));
  else {
    sprintf(buf, "'%s':illegal command error.\n", argv[0].c_str());
    notify(Playernum, Governor, buf);
  }

  /* compute the prompt and send to the player */
  d.out << do_prompt(d);
}

static void load_race_data() {
  Num_races = Numraces();
  for (int i = 1; i <= Num_races; i++) {
    getrace(&races[i - 1], i); /* allocates into memory */
    if (races[i - 1]->Playernum != i) {
      races[i - 1]->Playernum = i;
      putrace(races[i - 1]);
    }
  }
}

static void load_star_data() {
  int s, t;
  startype *star_arena;
  int pcount = 0;

  /* get star database */
  Planet_count = 0;
  getsdata(&Sdata);
  star_arena = (startype *)malloc(Sdata.numstars * sizeof(startype));
  for (s = 0; s < Sdata.numstars; s++) {
    Stars[s] = &star_arena[s]; /* Initialize star pointers */
  }
  for (s = 0; s < Sdata.numstars; s++) {
    getstar(&(Stars[s]), s);
    pcount += Stars[s]->numplanets;
  }

  auto planet_arena = (planettype *)malloc(pcount * sizeof(planettype));

  for (s = 0; s < Sdata.numstars; s++) {
    for (t = 0; t < Stars[s]->numplanets; t++) {
      // TODO(jeffbailey): This is a leak - will fix as part of global planet
      // array cleanup.
      planets[s][t] = &planet_arena[--pcount];
      planets[s][t] = new Planet(getplanet(s, t));
      if (planets[s][t]->type != PlanetType::ASTEROID) Planet_count++;
    }
  }
}

/* report back the update status */
static void GB_time(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  time_t clk = time(nullptr);
  notify(Playernum, Governor, start_buf);
  notify(Playernum, Governor, update_buf);
  notify(Playernum, Governor, segment_buf);
  sprintf(buf, "Current time    : %s", ctime(&clk));
  notify(Playernum, Governor, buf);
}

static void GB_schedule(const command_t &argv, GameObj &g) {
  player_t Playernum = g.player;
  governor_t Governor = g.governor;
  time_t clk = time(nullptr);
  sprintf(buf, "%d minute update intervals\n", update_time);
  notify(Playernum, Governor, buf);
  sprintf(buf, "%ld movement segments per update\n", segments);
  notify(Playernum, Governor, buf);
  sprintf(buf, "Current time    : %s", ctime(&clk));
  notify(Playernum, Governor, buf);
  sprintf(buf, "Next Segment %2d : %s",
          nsegments_done == segments ? 1 : nsegments_done + 1,
          ctime(&next_segment_time));
  notify(Playernum, Governor, buf);
  sprintf(buf, "Next Update %3d : %s", nupdates_done + 1,
          ctime(&next_update_time));
  notify(Playernum, Governor, buf);
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

void check_for_telegrams(int Playernum, int Governor) {
  sprintf(buf, "%s.%d.%d", TELEGRAMFL, Playernum, Governor);
  stat(buf, &sbuf);
  if (sbuf.st_size)
    notify(Playernum, Governor,
           "You have telegram(s) waiting. Use 'read' to read them.\n");
}

void kill_ship(int Playernum, shiptype *ship) {
  racetype *killer, *victim;
  shiptype *s;
  int sh;

  ship->special.mind.who_killed = Playernum;
  ship->alive = 0;
  ship->notified = 0; /* prepare the ship for recycling */

  if (ship->type != STYPE_POD && ship->type != OTYPE_FACTORY) {
    /* pods don't do things to morale, ditto for factories */
    victim = races[ship->owner - 1];
    if (victim->Gov_ship == ship->number) victim->Gov_ship = 0;
    if (!victim->God && Playernum != ship->owner && ship->type != OTYPE_VN) {
      killer = races[Playernum - 1];
      adjust_morale(killer, victim, (int)ship->build_cost);
      putrace(killer);
    } else if (ship->owner == Playernum && !ship->docked && Max_crew(ship)) {
      victim->morale -= 2 * ship->build_cost; /* scuttle/scrap */
    }
    putrace(victim);
  }

  if (ship->type == OTYPE_VN || ship->type == OTYPE_BERS) {
    getsdata(&Sdata);
    /* add ship to VN shit list */
    Sdata.VN_hitlist[ship->special.mind.who_killed - 1] += 1;

    /* keep track of where these VN's were shot up */

    if (Sdata.VN_index1[Playernum - 1] == -1)
      /* there's no star in the first index */
      Sdata.VN_index1[Playernum - 1] = ship->storbits;
    else if (Sdata.VN_index2[Playernum - 1] == -1)
      /* there's no star in the second index */
      Sdata.VN_index2[Playernum - 1] = ship->storbits;
    else {
      /* pick an index to supplant */
      if (random() & 01)
        Sdata.VN_index1[Playernum - 1] = ship->storbits;
      else
        Sdata.VN_index2[Playernum - 1] = ship->storbits;
    }
    putsdata(&Sdata);
  }

  if (ship->type == OTYPE_TOXWC && ship->whatorbits == ScopeLevel::LEVEL_PLAN) {
    auto planet = getplanet((int)ship->storbits, (int)ship->pnumorbits);
    planet.conditions[TOXIC] =
        MIN(100, planet.conditions[TOXIC] + ship->special.waste.toxic);
    putplanet(planet, Stars[ship->storbits], (int)ship->pnumorbits);
  }

  /* undock the stuff docked with it */
  if (ship->docked && ship->whatorbits != ScopeLevel::LEVEL_SHIP &&
      ship->whatdest == ScopeLevel::LEVEL_SHIP) {
    (void)getship(&s, (int)ship->destshipno);
    s->docked = 0;
    s->whatdest = ScopeLevel::LEVEL_UNIV;
    putship(s);
    free(s);
  }
  /* landed ships are killed */
  if (ship->ships) {
    sh = ship->ships;
    while (sh) {
      (void)getship(&s, sh);
      kill_ship(Playernum, s);
      putship(s);
      sh = s->nextship;
      free(s);
    }
  }
}

void compute_power_blocks() {
  const time_t clk = time(nullptr);
  int i, j, dummy[2];
  /* compute alliance block power */
  sprintf(Power_blocks.time, "%s", ctime(&clk));
  for (i = 1; i <= Num_races; i++) {
    dummy[0] = (Blocks[i - 1].invite[0] & Blocks[i - 1].pledge[0]);
    dummy[1] = (Blocks[i - 1].invite[1] & Blocks[i - 1].pledge[1]);
    Power_blocks.members[i - 1] = 0;
    Power_blocks.sectors_owned[i - 1] = 0;
    Power_blocks.popn[i - 1] = 0;
    Power_blocks.ships_owned[i - 1] = 0;
    Power_blocks.resource[i - 1] = 0;
    Power_blocks.fuel[i - 1] = 0;
    Power_blocks.destruct[i - 1] = 0;
    Power_blocks.money[i - 1] = 0;
    Power_blocks.systems_owned[i - 1] = Blocks[i - 1].systems_owned;
    Power_blocks.VPs[i - 1] = Blocks[i - 1].VPs;
    for (j = 1; j <= Num_races; j++)
      if (isset(dummy, j)) {
        Power_blocks.members[i - 1] += 1;
        Power_blocks.sectors_owned[i - 1] += Power[j - 1].sectors_owned;
        Power_blocks.money[i - 1] += Power[j - 1].money;
        Power_blocks.popn[i - 1] += Power[j - 1].popn;
        Power_blocks.ships_owned[i - 1] += Power[j - 1].ships_owned;
        Power_blocks.resource[i - 1] += Power[j - 1].resource;
        Power_blocks.fuel[i - 1] += Power[j - 1].fuel;
        Power_blocks.destruct[i - 1] += Power[j - 1].destruct;
      }
  }
}

/*utilities for dealing with ship lists */
void insert_sh_univ(struct stardata *sdata, shiptype *s) {
  s->nextship = sdata->ships;
  sdata->ships = s->number;
  s->whatorbits = ScopeLevel::LEVEL_UNIV;
}

void insert_sh_star(startype *star, shiptype *s) {
  s->nextship = star->ships;
  star->ships = s->number;
  s->whatorbits = ScopeLevel::LEVEL_STAR;
}

void insert_sh_plan(Planet *pl, shiptype *s) {
  s->nextship = pl->ships;
  pl->ships = s->number;
  s->whatorbits = ScopeLevel::LEVEL_PLAN;
}

void insert_sh_ship(shiptype *s, shiptype *s2) {
  s->nextship = s2->ships;
  s2->ships = s->number;
  s->whatorbits = ScopeLevel::LEVEL_SHIP;
  s->whatdest = ScopeLevel::LEVEL_SHIP;
  s->destshipno = s2->number;
}

void remove_sh_star(shiptype *s) {
  shipnum_t sh;
  shiptype *s2;

  getstar(&Stars[s->storbits], (int)s->storbits);
  sh = Stars[s->storbits]->ships;

  if (sh == s->number) {
    Stars[s->storbits]->ships = s->nextship;
    putstar(Stars[s->storbits], (int)(s->storbits));
  } else {
    while (sh != s->number) {
      (void)getship(&s2, sh);
      sh = s2->nextship;
      if (sh != s->number) free(s2);
    }
    s2->nextship = s->nextship;
    putship(s2);
    free(s2);
  }
  s->whatorbits = ScopeLevel::LEVEL_UNIV;
  s->nextship = 0;
}

void remove_sh_plan(shiptype *s) {
  shipnum_t sh;
  shiptype *s2;

  auto p = getplanet((int)s->storbits, (int)s->pnumorbits);
  sh = p.ships;

  if (sh == s->number) {
    p.ships = s->nextship;
    putplanet(p, Stars[s->storbits], (int)s->pnumorbits);
  } else {
    while (sh != s->number) {
      (void)getship(&s2, sh);
      sh = s2->nextship;
      if (sh != s->number) free(s2); /* don't free it if it is the s2 we want */
    }
    s2->nextship = s->nextship;
    putship(s2);
    free(s2);
  }
  s->nextship = 0;
  s->whatorbits = ScopeLevel::LEVEL_UNIV;
}

void remove_sh_ship(shiptype *s, shiptype *ship) {
  shiptype *s2;
  shipnum_t sh = ship->ships;

  if (sh == s->number)
    ship->ships = s->nextship;
  else {
    while (sh != s->number) {
      (void)getship(&s2, sh);
      sh = (int)(s2->nextship);
      if (sh != s->number) free(s2);
    }
    s2->nextship = s->nextship;
    putship(s2);
    free(s2);
  }
  s->nextship = 0;
  s->whatorbits =
      ScopeLevel::LEVEL_UNIV; /* put in limbo - wait for insert_sh.. */
}

static double GetComplexity(int ship) {
  shiptype s;

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
  s.size = ship_size(&s);
  s.base_mass = getmass(&s);
  s.mass = getmass(&s);

  return complexity(&s);
}

int ShipCompare(const void *S1, const void *S2) {
  const int *s1 = (const int *)S1;
  const int *s2 = (const int *)S2;
  return (int)(GetComplexity(*s1) - GetComplexity(*s2));
}

void SortShips(void) {
  int i;

  for (i = 0; i < NUMSTYPES; i++) ShipVector[i] = i;
  qsort(ShipVector, NUMSTYPES, sizeof(int), ShipCompare);
}

void warn_race(int who, char *message) {
  int i;

  for (i = 0; i <= MAXGOVERNORS; i++)
    if (races[who - 1]->governor[i].active) warn(who, i, message);
}

void warn(const player_t who, const governor_t governor,
          const std::string &message) {
  if (!notify(who, governor, message) && !notify(who, 0, message))
    push_telegram(who, governor, message);
}

void warn_star(int a, int b, int star, char *message) {
  int i;

  for (i = 1; i <= Num_races; i++)
    if (i != a && i != b && isset(Stars[star]->inhabited, i))
      warn_race(i, message);
}

void notify_star(int a, int g, int b, int star, char *message) {
  racetype *Race;

#ifdef MONITOR
  Race = races[0]; /* deity */
  if (Race->monitor || (a != 1 && b != 1)) notify_race(1, message);
#endif
  for (auto d : descriptor_list)
    if (d->connected && (d->player != a || d->governor != g) &&
        isset(Stars[star]->inhabited, d->player)) {
      queue_string(d, message);
    }
}

void adjust_morale(racetype *winner, racetype *loser, int amount) {
  winner->morale += amount;
  loser->morale -= amount;
  winner->points[loser->Playernum] += amount;
}

static std::string do_prompt(DescriptorData &d) {
  player_t Playernum = d.player;
  shiptype *s, *s2;
  std::stringstream prompt;

  if (d.level == ScopeLevel::LEVEL_UNIV) {
    prompt << boost::format(" ( [%d] / )\n") % Sdata.AP[Playernum - 1];
  } else if (d.level == ScopeLevel::LEVEL_STAR) {
    prompt << boost::format(" ( [%d] /%s )\n") %
                  Stars[d.snum]->AP[Playernum - 1] % Stars[d.snum]->name;
  } else if (d.level == ScopeLevel::LEVEL_PLAN) {
    prompt << boost::format(" ( [%d] /%s/%s )\n") %
                  Stars[d.snum]->AP[Playernum - 1] % Stars[d.snum]->name %
                  Stars[d.snum]->pnames[d.pnum];
  } else if (d.level == ScopeLevel::LEVEL_SHIP) {
    (void)getship(&s, d.shipno);
    switch (s->whatorbits) {
      case ScopeLevel::LEVEL_UNIV:
        prompt << boost::format(" ( [%d] /#%ld )\n") % Sdata.AP[Playernum - 1] %
                      d.shipno;
        break;
      case ScopeLevel::LEVEL_STAR:
        prompt << boost::format(" ( [%d] /%s/#%ld )\n") %
                      Stars[s->storbits]->AP[Playernum - 1] %
                      Stars[s->storbits]->name % d.shipno;
        break;
      case ScopeLevel::LEVEL_PLAN:
        prompt << boost::format(" ( [%d] /%s/%s/#%ld )\n") %
                      Stars[s->storbits]->AP[Playernum - 1] %
                      Stars[s->storbits]->name %
                      Stars[s->storbits]->pnames[d.pnum] % d.shipno;
        break;
      /* I put this mess in because of non-functioning prompts when you
         are in a ship within a ship, or deeper. I am certain this can be
         done more elegantly (a lot more) but I don't feel like trying
         that right now. right now I want it to function. Maarten */
      case ScopeLevel::LEVEL_SHIP:
        (void)getship(&s2, (int)s->destshipno);
        switch (s2->whatorbits) {
          case ScopeLevel::LEVEL_UNIV:
            prompt << boost::format(" ( [%d] /#%lu/#%lu )\n") %
                          Sdata.AP[Playernum - 1] % s->destshipno % d.shipno;
            break;
          case ScopeLevel::LEVEL_STAR:
            prompt << boost::format(" ( [%d] /%s/#%lu/#%lu )\n") %
                          Stars[s->storbits]->AP[Playernum - 1] %
                          Stars[s->storbits]->name % s->destshipno % d.shipno;
            break;
          case ScopeLevel::LEVEL_PLAN:
            prompt << boost::format(" ( [%d] /%s/%s/#%ld/#%ld )\n") %
                          Stars[s->storbits]->AP[Playernum - 1] %
                          Stars[s->storbits]->name %
                          Stars[s->storbits]->pnames[d.pnum] % s->destshipno %
                          d.shipno;
            break;
          case ScopeLevel::LEVEL_SHIP:
            while (s2->whatorbits == ScopeLevel::LEVEL_SHIP) {
              free(s2);
              (void)getship(&s2, (int)s2->destshipno);
            }
            switch (s2->whatorbits) {
              case ScopeLevel::LEVEL_UNIV:
                prompt << boost::format(" ( [%d] / /../#%ld/#%ld )\n") %
                              Sdata.AP[Playernum - 1] % s->destshipno %
                              d.shipno;
                break;
              case ScopeLevel::LEVEL_STAR:
                prompt << boost::format(" ( [%d] /%s/ /../#%ld/#%ld )\n") %
                              Stars[s->storbits]->AP[Playernum - 1] %
                              Stars[s->storbits]->name % s->destshipno %
                              d.shipno;
                break;
              case ScopeLevel::LEVEL_PLAN:
                prompt << boost::format(" ( [%d] /%s/%s/ /../#%ld/#%ld )\n") %
                              Stars[s->storbits]->AP[Playernum - 1] %
                              Stars[s->storbits]->name %
                              Stars[s->storbits]->pnames[d.pnum] %
                              s->destshipno % d.shipno;
                break;
              default:
                break;
            }
            free(s2);
            break;
        }
    }
    free(s);
  }
  return prompt.str();
}
