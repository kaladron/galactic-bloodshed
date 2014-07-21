// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#include "GB_server.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <unordered_map>

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

static long last_update_time;
static long last_segment_time;
static int nupdates_done; /* number of updates so far */

static int port; /* port selection */
static char start_buf[128];
static char update_buf[128];
static char segment_buf[128];

static const char *connect_fail = "Connection refused.\n";
static const char *flushed_message = "<Output Flushed>\n";
static const char *shutdown_message = "Shutdown ordered by deity - Bye\n";
static const char *already_on = "Connection refused.\n";

int sig_null();

struct text_block {
  int nchars;
  struct text_block *nxt;
  char *start;
  char *buf;
};

struct text_queue {
  struct text_block *head;
  struct text_block **tail;
};

static struct descriptor_data {
  int descriptor;
  int connected;
  int God;       /* deity status */
  int Playernum; /* race */
  int Governor;  /* governor/subcommander */
  char *output_prefix;
  char *output_suffix;
  int output_size;
  struct text_queue output;
  struct text_queue input;
  char *raw_input;
  char *raw_input_at;
  long last_time;
  int quota;
  struct descriptor_data *next;
  struct descriptor_data **prev;
} *descriptor_list = 0;

static int sock;
static int ndescriptors = 0;

static void set_signals(void);
static void queue_string(struct descriptor_data *d, const char *message);
static void queue_write(struct descriptor_data *, const char *, int);
static void add_to_queue(struct text_queue *, const char *, int);
static struct text_block *make_text_block(const char *, int);
static void help(struct descriptor_data *);
static void process_command(int, int, const char *);

struct descriptor_data *new_connection(int);
char *addrout(long);
void do_update(int);
void do_segment(int, int);
int make_socket(int);
void clearstrings(struct descriptor_data *);
void shutdownsock(struct descriptor_data *);
void freeqs(struct descriptor_data *);
void make_nonblocking(int);
struct timeval update_quotas(struct timeval, struct timeval);
int process_output(struct descriptor_data *);
void welcome_user(struct descriptor_data *);
int flush_queue(struct text_queue *, int);
void free_text_block(struct text_block *);
char *strsave(char *);
void process_commands(void);
int do_command(struct descriptor_data *, char *);
void goodbye_user(struct descriptor_data *);
void set_userstring(char **, char *);
void dump_users(struct descriptor_data *);
void check_connect(struct descriptor_data *, char *);
void close_sockets(void);
int process_input(struct descriptor_data *);
void force_output(void);
void help_user(struct descriptor_data *);
void parse_connect(char *, char *, char *);
void shovechars(int);
int msec_diff(struct timeval, struct timeval);
struct timeval msec_add(struct timeval, int);
void save_command(struct descriptor_data *, char *);
struct descriptor_data *initializesock(int);

static struct timeval timeval_sub(struct timeval now, struct timeval then);

#define MAX_COMMAND_LEN 512

typedef void (*CommandFunction)(const player_t, const governor_t, const int);
static const std::unordered_map<std::string, CommandFunction> *commands;

int main(int argc, char **argv) {
  int i;
  struct stat stbuf;
  FILE *sfile;

  commands =
      new std::unordered_map<std::string, CommandFunction>{ { "relation",
                                                              relation } };

  open_data_files();
  printf("      ***   Galactic Bloodshed ver %s ***\n\n", VERS);
  clk = time(0);
  printf("      %s", ctime(&clk));
#ifdef EXTERNAL_TRIGGER
  printf("      The update  password is '%s'.\n", UPDATE_PASSWORD);
  printf("      The segment password is '%s'.\n", SEGMENT_PASSWORD);
#endif
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
      if (fgets(dum, sizeof dum, sfile))
        nupdates_done = atoi(dum);
      if (fgets(dum, sizeof dum, sfile))
        last_update_time = atol(dum);
      if (fgets(dum, sizeof dum, sfile))
        next_update_time = atol(dum);
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
        if (fgets(dum, sizeof dum, sfile))
          nsegments_done = atoi(dum);
        if (fgets(dum, sizeof dum, sfile))
          last_segment_time = atol(dum);
        if (fgets(dum, sizeof dum, sfile))
          next_segment_time = atol(dum);
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
    setbit(Blocks[i - 1].invite, i);
    setbit(Blocks[i - 1].pledge, i);
  }
  Putblock(Blocks);
  compute_power_blocks();
  set_signals();
  shovechars(port);
  close_sockets();
  close_data_files();
  printf("Going down.\n");
  return (1);
}

void set_signals(void) { signal(SIGPIPE, SIG_IGN); }

void notify_race(int race, const char *message) {
  struct descriptor_data *d;
  if (update_flag)
    return;
  for (d = descriptor_list; d; d = d->next) {
    if (d->connected && d->Playernum == race) {
      queue_string(d, message);
    }
  }
}

int notify(int race, int gov, const char *message) {
  struct descriptor_data *d;
  int ok;

  if (update_flag)
    return 0;
  ok = 0;
  for (d = descriptor_list; d; d = d->next)
    if (d->connected && d->Playernum == race && d->Governor == gov) {
      queue_string(d, message);
      ok = 1;
    }
  return ok;
}

void d_think(int Playernum, int Governor, char *message) {
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next) {
    if (d->connected && d->Playernum == Playernum && d->Governor != Governor &&
        !races[d->Playernum - 1]->governor[d->Governor].toggle.gag) {
      queue_string(d, message);
    }
  }
}

void d_broadcast(int Playernum, int Governor, char *message) {
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next) {
    if (d->connected &&
        !(d->Playernum == Playernum && d->Governor == Governor) &&
        !races[d->Playernum - 1]->governor[d->Governor].toggle.gag) {
      queue_string(d, message);
    }
  }
}

void d_shout(int Playernum, int Governor, char *message) {
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next) {
    if (d->connected &&
        !(d->Playernum == Playernum && d->Governor == Governor)) {
      queue_string(d, message);
    }
  }
}

void d_announce(int Playernum, int Governor, int star, char *message) {
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next) {
    if (d->connected &&
        !(d->Playernum == Playernum && d->Governor == Governor) &&
        (isset(Stars[star]->inhabited, d->Playernum) ||
         races[d->Playernum - 1]->God) &&
        Dir[d->Playernum - 1][d->Governor].snum == star &&
        !races[d->Playernum - 1]->governor[d->Governor].toggle.gag) {
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

int msec_diff(struct timeval now, struct timeval then) {
  return ((now.tv_sec - then.tv_sec) * 1000 +
          (now.tv_usec - then.tv_usec) / 1000);
}

struct timeval msec_add(struct timeval t, int x) {
  t.tv_sec += x / 1000;
  t.tv_usec += (x % 1000) * 1000;
  if (t.tv_usec >= 1000000) {
    t.tv_sec += t.tv_usec / 1000000;
    t.tv_usec = t.tv_usec % 1000000;
  }
  return t;
}

void shovechars(int port) {
  fd_set input_set, output_set;
  long now, go_time;
  struct timeval last_slice, current_time;
  struct timeval next_slice;
  struct timeval timeout, slice_timeout;
  int maxd, i;
  struct descriptor_data *d, *dnext;
  struct descriptor_data *newd;
  int avail_descriptors;

  go_time = 0;
  sock = make_socket(port);
  maxd = sock + 1;
  gettimeofday(&last_slice, (struct timezone *)0);

  avail_descriptors = getdtablesize() - 4;

  if (!shutdown_flag)
    post("Server started\n", ANNOUNCE);
  for (i = 0; i <= ANNOUNCE; i++)
    newslength[i] = Newslength(i);

  while (!shutdown_flag) {
    fflush(stdout);
    gettimeofday(&current_time, (struct timezone *)0);
    last_slice = update_quotas(last_slice, current_time);

    process_commands();

    if (shutdown_flag)
      break;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    next_slice = msec_add(last_slice, COMMAND_TIME_MSEC);
    slice_timeout = timeval_sub(next_slice, current_time);

    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    if (ndescriptors < avail_descriptors)
      FD_SET(sock, &input_set);
    for (d = descriptor_list; d; d = d->next) {
      if (d->input.head)
        timeout = slice_timeout;
      else
        FD_SET(d->descriptor, &input_set);
      if (d->output.head)
        FD_SET(d->descriptor, &output_set);
    }

    if (select(maxd, &input_set, &output_set, (fd_set *)0, &timeout) < 0) {
      if (errno != EINTR) {
        perror("select");
        return;
      }
      (void)time(&now);
    } else {

      (void)time(&now);

      if (FD_ISSET(sock, &input_set)) {
        if (!(newd = new_connection(sock))) {
          if (errno && errno != EINTR && errno != EMFILE) {
            perror("new_connection");
            return;
          }
        } else {
          if (newd->descriptor >= maxd)
            maxd = newd->descriptor + 1;
        }
      }

      for (d = descriptor_list; d; d = dnext) {
        dnext = d->next;
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
}

void do_next_thing(void) {
  clk = time(0);
  if (nsegments_done < segments)
    do_segment(0, 1);
  else
    do_update(0);
}

int make_socket(int port) {
  int s;
  struct sockaddr_in server;
  int opt;

  s = socket(AF_INET, SOCK_STREAM, 0);
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
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  if (bind(s, (struct sockaddr *)&server, sizeof(server))) {
    perror("binding stream socket");
    close(s);
    exit(4);
  }
  listen(s, 5);
  return s;
}

struct timeval update_quotas(struct timeval last, struct timeval current) {
  int nslices;
  struct descriptor_data *d;

  nslices = msec_diff(current, last) / COMMAND_TIME_MSEC;

  if (nslices > 0) {
    for (d = descriptor_list; d; d = d->next) {
      d->quota += COMMANDS_PER_TIME * nslices;
      if (d->quota > COMMAND_BURST_SIZE)
        d->quota = COMMAND_BURST_SIZE;
    }
  }
  return msec_add(last, nslices * COMMAND_TIME_MSEC);
}

struct descriptor_data *new_connection(int sock) {
  int newsock;
  struct sockaddr_in addr;
  socklen_t addr_len;

  addr_len = sizeof(addr);
  newsock = accept(sock, (struct sockaddr *)&addr, &addr_len);
  if (newsock <= 0) {
    return 0;
  } else {
#ifdef ACCESS_CHECK
    if (address_ok(&addr)) {
      fprintf(stderr, "ACCEPT from %s(%d) on descriptor %d\n",
              addrout(ntohl(addr.sin_addr.s_addr)), ntohs(addr.sin_port),
              newsock);
      return initializesock(newsock);
    } else {
      write(newsock, "Unauthorized Access.\n", 21);
      fprintf(stderr, "REJECT from %s(%d) on descriptor %d\n",
              addrout(ntohl(addr.sin_addr.s_addr)), ntohs(addr.sin_port),
              newsock);
      shutdown(newsock, 2);
      close(newsock);
      errno = 0;
      return 0;
    }
#else
    fprintf(stderr, "ACCEPT from %s(%d) on descriptor %d\n",
            addrout(ntohl(addr.sin_addr.s_addr)), ntohs(addr.sin_port),
            newsock);
    return initializesock(newsock);
#endif
  }
}

#ifdef ACCESS_CHECK

static int naddresses = 0;
typedef struct access {
  struct in_addr ac_addr;
  int access_value;
} ac_t;
static ac_t *ac_tab = (ac_t *)NULL;

int address_ok(struct sockaddr_in *ap) {
  int i;
  ac_t *acp;
  static int ainit = 0;
  FILE *afile;
  char *cp, ibuf[64];
  int aval;
  unsigned long ina;

  if (!ainit) {
    ainit = 1;
    if ((afile = fopen(ADDRESSFL, "r"))) {
      while (fgets(ibuf, sizeof ibuf, afile)) {
        cp = ibuf;
        aval = 1;
        if (*cp == '#' || *cp == '\n' || *cp == '\r')
          continue;
        if (*cp == '!') {
          cp++;
          aval = 0;
        }
        ina = inet_addr(cp);
        if (ina != -1)
          add_address(ina, aval);
      }
      fclose(afile);
      /* Always allow localhost. */
      add_address(inet_addr("127"), 1);
    } else {
      /* Allow anything. */
      add_address(inet_addr("0"), 1);
    }
  }

  acp = ac_tab;
  for (i = 0; i < naddresses; i++, acp++) {
    if (address_match(&ap->sin_addr.s_addr, &acp->ac_addr))
      return acp->access_value;
  }
  return 0;
}

address_match(struct in_addr *addr, struct in_addr *apat) {
  /* Ugh!  There HAS to be a better way... */

  if (!apat->S_un.S_un_b.s_b1) {
    if (apat->S_un.S_un_b.s_b4 &&
        apat->S_un.S_un_b.s_b4 != addr->S_un.S_un_b.s_b1)
      return 0;
    return 1;
  } else {
    if (apat->S_un.S_un_b.s_b1 != addr->S_un.S_un_b.s_b1)
      return 0;
  }
  if (!apat->S_un.S_un_b.s_b2) {
    if (apat->S_un.S_un_b.s_b4 &&
        apat->S_un.S_un_b.s_b4 != addr->S_un.S_un_b.s_b2)
      return 0;
    return 1;
  } else {
    if (apat->S_un.S_un_b.s_b2 != addr->S_un.S_un_b.s_b2)
      return 0;
  }
  if (!apat->S_un.S_un_b.s_b3) {
    if (apat->S_un.S_un_b.s_b4 &&
        apat->S_un.S_un_b.s_b4 != addr->S_un.S_un_b.s_b3)
      return 0;
    return 1;
  } else {
    if (apat->S_un.S_un_b.s_b3 != addr->S_un.S_un_b.s_b3)
      return 0;
  }
  if (apat->S_un.S_un_b.s_b4 != addr->S_un.S_un_b.s_b4)
    return 0;
  return 1;
}

void add_address(unsigned long ina, int aval) {
  ac_t *nac_t;

  if (naddresses > 0)
    nac_t = (ac_t *)realloc(ac_tab, sizeof(ac_t) * (naddresses + 1));
  else
    nac_t = (ac_t *)calloc(1, sizeof(ac_t));
  if (!nac_t) {
    printf("add_address: Out of memory.\n");
    return;
  }
  ac_tab = nac_t;
  nac_t = &ac_tab[naddresses++];
  nac_t->ac_addr.s_addr = ina;
  nac_t->access_value = aval;
}
#endif

char *addrout(long a) {
  static char outbuf[1024];

  sprintf(outbuf, "%ld.%ld.%ld.%ld", (a >> 24) & 0xff, (a >> 16) & 0xff,
          (a >> 8) & 0xff, a & 0xff);
  return outbuf;
}

void clearstrings(struct descriptor_data *d) {
  if (d->output_prefix) {
    free(d->output_prefix);
    d->output_prefix = 0;
  }
  if (d->output_suffix) {
    free(d->output_suffix);
    d->output_suffix = 0;
  }
}

void shutdownsock(struct descriptor_data *d) {
  if (d->connected) {
    fprintf(stderr, "DISCONNECT %d Race=%d Governor=%d\n", d->descriptor,
            d->Playernum, d->Governor);
  } else {
    fprintf(stderr, "DISCONNECT %d never connected\n", d->descriptor);
  }
  clearstrings(d);
  shutdown(d->descriptor, 2);
  close(d->descriptor);
  freeqs(d);
  *d->prev = d->next;
  if (d->next)
    d->next->prev = d->prev;
  free(d);
  ndescriptors--;
}

struct descriptor_data *initializesock(int s) {
  struct descriptor_data *d;

  ndescriptors++;
  d = (struct descriptor_data *)malloc(sizeof(struct descriptor_data));
  d->descriptor = s;
  d->connected = 0;
  make_nonblocking(s);
  d->output_prefix = 0;
  d->output_suffix = 0;
  d->output_size = 0;
  d->output.head = 0;
  d->output.tail = &d->output.head;
  d->input.head = 0;
  d->input.tail = &d->input.head;
  d->raw_input = 0;
  d->raw_input_at = 0;
  d->quota = COMMAND_BURST_SIZE;
  d->last_time = 0;
  if (descriptor_list)
    descriptor_list->prev = &d->next;
  d->next = descriptor_list;
  d->prev = &descriptor_list;
  descriptor_list = d;

  welcome_user(d);
  return d;
}

static struct text_block *make_text_block(const char *s, int n) {
  struct text_block *p;

  p = (struct text_block *)malloc(sizeof(struct text_block));
  p->buf = (char *)malloc(n * sizeof(char));
  bcopy(s, p->buf, n);
  p->nchars = n;
  p->start = p->buf;
  p->nxt = 0;
  return p;
}

void free_text_block(struct text_block *t) {
  free(t->buf);
  free((char *)t);
}

static void add_to_queue(struct text_queue *q, const char *b, int n) {
  struct text_block *p;

  if (n == 0)
    return;

  p = make_text_block(b, n);
  p->nxt = 0;
  *q->tail = p;
  q->tail = &p->nxt;
}

int flush_queue(struct text_queue *q, int n) {
  struct text_block *p;
  int really_flushed = 0;

  n += strlen(flushed_message);

  while (n > 0 && (p = q->head)) {
    n -= p->nchars;
    really_flushed += p->nchars;
    q->head = p->nxt;
    free_text_block(p);
  }
  p = make_text_block(flushed_message, strlen(flushed_message));
  p->nxt = q->head;
  q->head = p;
  if (!p->nxt)
    q->tail = &p->nxt;
  really_flushed -= p->nchars;
  return really_flushed;
}

static void queue_write(struct descriptor_data *d, const char *b, int n) {
  int space;

  space = MAX_OUTPUT - d->output_size - n;
  if (space < 0)
    d->output_size -= flush_queue(&d->output, -space);
  add_to_queue(&d->output, b, n);
  d->output_size += n;
}

static void queue_string(struct descriptor_data *d, const char *s) {
  queue_write(d, s, strlen(s));
}

int process_output(struct descriptor_data *d) {
  struct text_block **qp, *cur;
  int cnt;

  for (qp = &d->output.head; (cur = *qp);) {
    cnt = write(d->descriptor, cur->start, cur->nchars);
    if (cnt < 0) {
      if (errno == EWOULDBLOCK)
        return 1;
      d->connected = 0; /* added this */
      return 0;
    }
    d->output_size -= cnt;
    if (cnt == cur->nchars) {
      if (!cur->nxt)
        d->output.tail = qp;
      *qp = cur->nxt;
      free_text_block(cur);
      continue; /* do not adv ptr */
    }
    cur->nchars -= cnt;
    cur->start += cnt;
    break;
  }
  return 1;
}

void force_output(void) {
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
    if (d->connected)
      (void)process_output(d);
}

void make_nonblocking(int s) {
  if (fcntl(s, F_SETFL, O_NDELAY) == -1) {
    perror("make_nonblocking: fcntl");
    exit(0);
  }
}

void freeqs(struct descriptor_data *d) {
  struct text_block *cur, *next;

  cur = d->output.head;
  while (cur) {
    next = cur->nxt;
    free_text_block(cur);
    cur = next;
  }
  d->output.head = 0;
  d->output.tail = &d->output.head;

  cur = d->input.head;
  while (cur) {
    next = cur->nxt;
    free_text_block(cur);
    cur = next;
  }
  d->input.head = 0;
  d->input.tail = &d->input.head;

  if (d->raw_input)
    free(d->raw_input);
  d->raw_input = 0;
  d->raw_input_at = 0;
}

void welcome_user(struct descriptor_data *d) {
  FILE *f;
  char *p;

  sprintf(buf, "***   Welcome to Galactic Bloodshed %s ***\n", VERS);
  queue_string(d, buf);

  if ((f = fopen(WELCOME_FILE, "r")) != NULL) {
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

void help_user(struct descriptor_data *d) {
  FILE *f;
  char *p;

  if ((f = fopen(HELP_FILE, "r")) != NULL) {
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

void goodbye_user(struct descriptor_data *d) {
  if (d->connected) /* this can happen, especially after updates */
    write(d->descriptor, LEAVE_MESSAGE, strlen(LEAVE_MESSAGE));
}

char *strsave(char *s) {
  char *p;

  p = (char *)malloc((strlen(s) + 1) * sizeof(char));

  if (p)
    strcpy(p, s);
  return p;
}

void save_command(struct descriptor_data *d, char *command) {
  add_to_queue(&d->input, command, strlen(command) + 1);
}

int process_input(struct descriptor_data *d) {
  int got;
  char *p, *pend, *q, *qend;

  got = read(d->descriptor, buf, sizeof buf);
  if (got <= 0)
    return 0;
  if (!d->raw_input) {
    d->raw_input = (char *)malloc(MAX_COMMAND_LEN * sizeof(char));
    d->raw_input_at = d->raw_input;
  }
  p = d->raw_input_at;
  pend = d->raw_input + MAX_COMMAND_LEN - 1;
  for (q = buf, qend = buf + got; q < qend; q++) {
    if (*q == '\n') {
      *p = '\0';
      if (p > d->raw_input)
        save_command(d, d->raw_input);
      p = d->raw_input;
    } else if (p < pend && isascii(*q) && isprint(*q)) {
      *p++ = *q;
    }
  }
  if (p > d->raw_input) {
    d->raw_input_at = p;
  } else {
    free(d->raw_input);
    d->raw_input = 0;
    d->raw_input_at = 0;
  }
  return 1;
}

void set_userstring(char **userstring, char *comm) {
  if (*userstring) {
    free(*userstring);
    *userstring = 0;
  }
  while (*comm && isascii(*comm) && isspace(*comm))
    comm++;
  if (*comm)
    *userstring = strsave(comm);
}

void process_commands(void) {
  int nprocessed;
  long now;
  struct descriptor_data *d, *dnext;
  struct text_block *t;

  (void)time(&now);

  do {
    nprocessed = 0;
    for (d = descriptor_list; d; d = dnext) {
      dnext = d->next;
      if (d->quota > 0 && (t = d->input.head)) {
        d->quota--;
        nprocessed++;

        if (!do_command(d, t->start)) {
          shutdownsock(d);
        } else {
          d->last_time = now; /* experimental code */
          d->input.head = t->nxt;
          if (!d->input.head)
            d->input.tail = &d->input.head;
          free_text_block(t);
          d->last_time = now; /* experimental code */
        }
      }
    }
  } while (nprocessed > 0);
}

int do_command(struct descriptor_data *d, char *comm) {
  char *string;
  int parse_exit = 0, i;

  argn = 0;
  /* Main processing loop. When command strings are sent from the client,
     they are processed here. Responses are sent back to the client via notify.
     The client will then process the strings in whatever manner is expected. */

  /* check to see if there are a few words typed out, usually for the help
   * command */
  string = comm;
  while (!parse_exit) {
    i = 0;
    while (!isspace(*string) && (*string != '\0') && (i < COMMANDSIZE))
      args[argn][i++] = (*string++);
    args[argn][i] = '\0';
    while ((*string) == ' ')
      string++;
    if ((*string == '\0') || (argn >= MAXARGS))
      parse_exit = 1;
    argn++;
  }
  for (i = argn; i < MAXARGS; i++)
    args[i][0] = '\0';

  if (!strcmp(args[0], QUIT_COMMAND)) {
    goodbye_user(d);
    return 0;
  } else if (d->connected && !strcmp(args[0], WHO_COMMAND)) {
    dump_users(d);
  } else if (!strcmp(args[0], HELP_COMMAND)) {
    help(d);
  } else if (d->connected && d->God && !strcmp(args[0], EMULATE_COMMAND)) {
    d->Playernum = atoi(args[1]);
    d->Governor = atoi(args[2]);
    sprintf(buf, "Emulating %s \"%s\" [%d,%d]\n", races[d->Playernum - 1]->name,
            races[d->Playernum - 1]->governor[d->Governor].name, d->Playernum,
            d->Governor);
    queue_string(d, buf);
  } else {
    if (d->connected) {
      /* GB command parser */
      process_command(d->Playernum, d->Governor, comm);
    } else {
      check_connect(
          d, comm); /* Logs player into the game, connects
                          if the password given by *command is a player's */
      if (!d->connected) {
        goodbye_user(d);
      } else {
        check_for_telegrams(d->Playernum, d->Governor);
        /* set the scope to home upon login */
        argn = 1;
        strcpy(args[0], "cs");
        process_command(d->Playernum, d->Governor, "cs");
      }
    }
  }
  return 1;
}

void check_connect(struct descriptor_data *d, char *message) {
  char race_password[MAX_COMMAND_LEN];
  char gov_password[MAX_COMMAND_LEN];
  int i, j;
  int Playernum, Governor;
  racetype *r;
  struct descriptor_data *d0, *dnext;

  parse_connect(message, race_password, gov_password);

#ifdef EXTERNAL_TRIGGER
  if (!strcmp(password, SEGMENT_PASSWORD)) {
    do_segment(1, 0);
    return;
  } else if (!strcmp(password, UPDATE_PASSWORD)) {
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
    for (d0 = descriptor_list; d0; d0 = dnext) {
      dnext = d0->next;
      if (d0->connected && d0->Playernum == Playernum &&
          d0->Governor == Governor) {
        queue_string(d, already_on);
        return;
      }
    }

    fprintf(stderr, "CONNECTED %s \"%s\" [%d,%d] on descriptor %d\n", r->name,
            r->governor[j].name, Playernum, Governor, d->descriptor);
    d->connected = 1;

    d->God = r->God;
    d->Playernum = Playernum;
    d->Governor = Governor;

    sprintf(buf, "\n%s \"%s\" [%d,%d] logged on.\n", r->name,
            r->governor[j].name, Playernum, Governor);
    notify_race(Playernum, buf);
    sprintf(buf, "You are %s.\n",
            r->governor[j].toggle.invisible ? "invisible" : "visible");
    notify(Playernum, Governor, buf);

    GB_time(Playernum, Governor);
    sprintf(buf, "\nLast login      : %s", ctime(&(r->governor[j].login)));
    notify(Playernum, Governor, buf);
    r->governor[j].login = time(0);
    putrace(r);
    if (!r->Gov_ship) {
      sprintf(buf, "You have no Governmental Center.  No action points will be "
                   "produced\nuntil you build one and designate a capital.\n");
      notify(Playernum, Governor, buf);
    } else {
      sprintf(buf, "Government Center #%lu is active.\n", r->Gov_ship);
      notify(Playernum, Governor, buf);
    }
    sprintf(buf, "     Morale: %ld\n", r->morale);
    notify(Playernum, Governor, buf);
    treasury(Playernum, Governor);
  }
}

void do_update(int override) {
  int i;
  FILE *sfile;
  struct stat stbuf;
  int fakeit;

  fakeit = (!override && stat(NOGOFL, &stbuf) >= 0);

  sprintf(buf, "%sDOING UPDATE...\n", ctime(&clk));
  if (!fakeit) {
    for (i = 1; i <= Num_races; i++)
      notify_race(i, buf);
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

  if (!fakeit)
    nupdates_done++;

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
  if (!fakeit)
    do_turn(1);
  update_flag = 0;
  clk = time(0);
  sprintf(buf, "%sUpdate %d finished\n", ctime(&clk), nupdates_done);
  if (!fakeit) {
    for (i = 1; i <= Num_races; i++)
      notify_race(i, buf);
    force_output();
  }
}

void do_segment(int override, int segment) {
  int i;
  FILE *sfile;
  struct stat stbuf;
  int fakeit;

  fakeit = (!override && stat(NOGOFL, &stbuf) >= 0);

  if (!override && segments <= 1)
    return;

  sprintf(buf, "%sDOING MOVEMENT...\n", ctime(&clk));
  if (!fakeit) {
    for (i = 1; i <= Num_races; i++)
      notify_race(i, buf);
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
  if (!fakeit)
    do_turn(0);
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
  clk = time(0);
  sprintf(buf, "%sSegment finished\n", ctime(&clk));
  if (!fakeit) {
    for (i = 1; i <= Num_races; i++)
      notify_race(i, buf);
    force_output();
  }
}

void parse_connect(char *message, char *race_pass, char *gov_pass) {
  char *p;
  char *q;
  /* race password */
  while (*message && isascii(*message) && isspace(*message))
    message++;
  p = race_pass;
  while (*message && isascii(*message) && !isspace(*message))
    *p++ = *message++;
  *p = '\0';
  /* governor password */
  while (*message && isascii(*message) && isspace(*message))
    message++;
  q = gov_pass;
  while (*message && isascii(*message) && !isspace(*message))
    *q++ = *message++;
  *q = '\0';
}

void close_sockets(void) {
  struct descriptor_data *d, *dnext;
  /* post message into news file */
  post(shutdown_message, ANNOUNCE);

  for (d = descriptor_list; d; d = dnext) {
    dnext = d->next;
    write(d->descriptor, shutdown_message, strlen(shutdown_message));
    if (shutdown(d->descriptor, 2) < 0)
      perror("shutdown");
    close(d->descriptor);
  }
  close(sock);
}

void dump_users(struct descriptor_data *e) {
  struct descriptor_data *d;
  long now;
  racetype *r;
  int God = 0;
  int coward_count = 0;

  (void)time(&now);
  sprintf(buf, "Current Players: %s", ctime(&now));
  queue_string(e, buf);
  if (e->Playernum) {
    r = races[e->Playernum - 1];
    God = r->God;
  } else
    return;

  for (d = descriptor_list; d; d = d->next) {
    if (d->connected && !d->God) {
      r = races[d->Playernum - 1];
      if (!r->governor[d->Governor].toggle.invisible ||
          e->Playernum == d->Playernum || God) {
        sprintf(temp, "\"%s\"", r->governor[d->Governor].name);
        sprintf(buf, "%20.20s %20.20s [%2d,%2d] %4lds idle %-4.4s %s %s\n",
                r->name, temp, d->Playernum, d->Governor, now - d->last_time,
                God ? Stars[Dir[d->Playernum - 1][d->Governor].snum]->name
                    : "    ",
                (r->governor[d->Governor].toggle.gag ? "GAG" : "   "),
                (r->governor[d->Governor].toggle.invisible ? "INVISIBLE" : ""));
        queue_string(e, buf);
      } else if (!God) /* deity lurks around */
        coward_count++;

      if ((now - d->last_time) > DISCONNECT_TIME)
        d->connected = 0;
    }
  }
#ifdef SHOW_COWARDS
  sprintf(buf, "And %d coward%s.\n", coward_count,
          (coward_count == 1) ? "" : "s");
  queue_string(e, buf);
#endif
  queue_string(e, "Finished.\n");
}

static void process_command(int Playernum, int Governor, const char *comm) {
  int j, God, Guest;
  racetype *r;

  /*determine which routine to go to and what action to take */
  /* target routine is specified by the first substring, other options
  are the substrings which follow */

  r = races[Playernum - 1];
  God = r->God;
  Guest = r->Guest;

  const char *string = comm;
  while (*string && *string != ' ')
    string++;

  auto command = commands->find(args[0]);
  if (command != commands->end()) {
    command->second(Playernum, Governor, 0);
  } else if (match(args[0], "announce")) /* keep this at the front */
    announce(Playernum, Governor, string, ANN);
  else if (match(args[0], "allocate"))
    allocateAPs(Playernum, Governor, 0);
  else if (match(args[0], "analysis"))
    analysis(Playernum, Governor, 0);
  else if (match(args[0], "arm"))
    arm(Playernum, Governor, 0, 1);
  else if (match(args[0], "disarm"))
    arm(Playernum, Governor, 0, 0);
  else if (match(args[0], "assault"))
    dock(Playernum, Governor, 1, 1);
  else if (match(args[0], "autoreport"))
    autoreport(Playernum, Governor, 0);
  else if (match(args[0], "build"))
    build(Playernum, Governor, 1);
#ifdef MARKET
  else if (match(args[0], "bid"))
    bid(Playernum, Governor, 0);
  else if (match(args[0], "sell"))
    sell(Playernum, Governor, 20);
#endif
  else if (match(args[0], "bless") && God)
    bless(Playernum, Governor, 0);
  else if (match(args[0], "'") || match(args[0], "broadcast"))
    announce(Playernum, Governor, string, BROADCAST);
  else if (match(args[0], "shout") && God)
    announce(Playernum, Governor, string, SHOUT);
  else if (match(args[0], "think"))
    announce(Playernum, Governor, string, THINK);
  else if (match(args[0], "block"))
    block(Playernum, Governor, 0);
  else if (match(args[0], "bombard") && !Guest)
    bombard(Playernum, Governor, 1);
  else if (match(args[0], "cs"))
    cs(Playernum, Governor, 0);
  else if (match(args[0], "capital"))
    capital(Playernum, Governor, 50);
  else if (match(args[0], "capture"))
    capture(Playernum, Governor, 1);
  else if (match(args[0], "center"))
    center(Playernum, Governor, 0);
  else if (match(args[0], "declare"))
    declare(Playernum, Governor, 1);
#ifdef DEFENSE
  else if (match(args[0], "defend"))
    defend(Playernum, Governor, 1);
#endif
  else if (match(args[0], "detonate") && !Guest)
    detonate(Playernum, Governor, 0);
  else if (match(args[0], "distance"))
    distance(Playernum, Governor, 0);
  else if (match(args[0], "dismount"))
    mount(Playernum, Governor, 0, 0);
  else if (match(args[0], "dissolve") && !Guest)
    dissolve(Playernum, Governor);
  else if (match(args[0], "dock"))
    dock(Playernum, Governor, 0, 0);
  else if (match(args[0], "dump") && !Guest)
    dump(Playernum, Governor, 10);
  else if (match(args[0], "enslave"))
    enslave(Playernum, Governor, 2);
  else if (match(args[0], "examine"))
    examine(Playernum, Governor, 0);
  else if (match(args[0], "explore"))
    exploration(Playernum, Governor, 0);
  else if (match(args[0], "cew"))
    fire(Playernum, Governor, 1, 1);
  else if (match(args[0], "fire") && !Guest)
    fire(Playernum, Governor, 1, 0);
  else if (match(args[0], "fuel"))
    proj_fuel(Playernum, Governor, 0);
  else if (match(args[0], "governors") || match(args[0], "appoint") ||
           match(args[0], "revoke"))
    governors(Playernum, Governor, 0);
  else if (match(args[0], "grant"))
    grant(Playernum, Governor, 0);
  else if (match(args[0], "give") && !Guest)
    give(Playernum, Governor, 5);
  else if (match(args[0], "highlight"))
    highlight(Playernum, Governor, 0);
  else if (match(args[0], "identify") || match(args[0], "whois"))
    whois(Playernum, Governor, 0);
#ifdef MARKET
  else if (match(args[0], "insurgency"))
    insurgency(Playernum, Governor, 10);
#endif
  else if (match(args[0], "invite"))
    invite(Playernum, Governor, 0, 1);
  else if (match(args[0], "jettison"))
    jettison(Playernum, Governor, 0);
  else if (match(args[0], "land"))
    land(Playernum, Governor, 1);
  else if (match(args[0], "launch"))
    launch(Playernum, Governor, 0);
  else if (match(args[0], "load"))
    load(Playernum, Governor, 0, 0);
  else if (match(args[0], "map"))
    map(Playernum, Governor, 0);
  else if (match(args[0], "mobilize"))
    mobilize(Playernum, Governor, 1);
  else if (match(args[0], "move"))
    move_popn(Playernum, Governor, CIV);
  else if (match(args[0], "deploy"))
    move_popn(Playernum, Governor, MIL);
  else if (match(args[0], "make"))
    make_mod(Playernum, Governor, 0, 0);
  else if (match(args[0], "modify"))
    make_mod(Playernum, Governor, 0, 1);
  else if (match(args[0], "mount"))
    mount(Playernum, Governor, 0, 1);
  else if (match(args[0], "motto"))
    motto(Playernum, Governor, 0, comm);
  else if (match(args[0], "name"))
    name(Playernum, Governor, 0);
  else if (match(args[0], "orbit"))
    orbit(Playernum, Governor, 0);
  else if (match(args[0], "order"))
    order(Playernum, Governor, 1);
  else if (match(args[0], "page"))
    page(Playernum, Governor, !God);
  else if (match(args[0], "pay") && !Guest)
    pay(Playernum, Governor, 0);
  else if (match(args[0], "colonies"))
    colonies(Playernum, Governor, 0, 0);
  else if (match(args[0], "personal"))
    personal(Playernum, Governor, string);
  else if (match(args[0], "pledge"))
    pledge(Playernum, Governor, 0, 1);
  else if (match(args[0], "power"))
    power(Playernum, Governor, 0);
  else if (match(args[0], "post"))
    send_message(Playernum, Governor, 0, 1);
  else if (match(args[0], "profile"))
    profile(Playernum, Governor, 0);
  else if (match(args[0], "production"))
    colonies(Playernum, Governor, 0, 1);
  else if (match(args[0], "purge") && God)
    purge();
  else if (match(args[0], "fix") && God)
    fix(Playernum, Governor);
  else if (match(args[0], "read"))
    read_messages(Playernum, Governor, 0);
  else if (match(args[0], "@@reset") && God) {
    for (j = 1; j <= Num_races; j++)
      notify_race(j, "DOING RESET...\n");
    force_output();
    load_race_data();
    load_star_data();
    do_reset(1);
  } else if (match(args[0], "report"))
    rst(Playernum, Governor, 0, 0);
  else if (match(args[0], "factories"))
    rst(Playernum, Governor, 0, 6);
  else if (match(args[0], "repair"))
    repair(Playernum, Governor, 0);
  else if (match(args[0], "route"))
    route(Playernum, Governor, 0);
  else if (match(args[0], "@@shutdown") && God) {
    shutdown_flag = 1;
    notify(Playernum, Governor, "Doing shutdown.\n");
  } else if (match(args[0], "@@update") && God)
    do_update(1);
  else if (match(args[0], "@@segment") && God)
    do_segment(1, atoi(args[1]));
  else if (match(args[0], "stock"))
    rst(Playernum, Governor, 0, 1);
  else if (match(args[0], "stats"))
    rst(Playernum, Governor, 0, 4);
  else if (match(args[0], "ship"))
    rst(Playernum, Governor, 0, 3);
  else if (match(args[0], "survey"))
    survey(Playernum, Governor, 0, 0);
  else if (match(args[0], "client_survey"))
    survey(Playernum, Governor, 0, 1);
  else if (match(args[0], "send"))
    send_message(Playernum, Governor, !God, 0);
  else if (match(args[0], "scrap") && (argn > 1))
    scrap(Playernum, Governor, 1);
  else if (match(args[0], "stars"))
    star_locations(Playernum, Governor, 0);
  else if (match(args[0], "status"))
    tech_status(Playernum, Governor, 0);
  else if (match(args[0], "tactical"))
    rst(Playernum, Governor, 0, 2);
#ifdef MARKET
  else if (match(args[0], "tax"))
    tax(Playernum, Governor, 0);
#endif
  else if (match(args[0], "technology"))
    technology(Playernum, Governor, 1);
  else if (match(args[0], "toggle"))
    toggle(Playernum, Governor, 0);
  else if (match(args[0], "toxicity"))
    toxicity(Playernum, Governor, 1);
#ifdef MARKET
  else if (match(args[0], "treasury"))
    treasury(Playernum, Governor);
#endif
  else if (match(args[0], "transfer") && !Guest)
    transfer(Playernum, Governor, 1);
  else if (match(args[0], "unload"))
    load(Playernum, Governor, 0, 1);
  else if (match(args[0], "undock"))
    launch(Playernum, Governor, 1);
  else if (match(args[0], "uninvite"))
    invite(Playernum, Governor, 0, 0);
  else if (match(args[0], "unpledge"))
    pledge(Playernum, Governor, 0, 0);
  else if (match(args[0], "upgrade"))
    upgrade(Playernum, Governor, 1);
  else if (match(args[0], "victory"))
    victory(Playernum, Governor, 0);
  else if (match(args[0], "walk"))
    walk(Playernum, Governor, 1);
  else if (match(args[0], "weapons"))
    rst(Playernum, Governor, 0, 5);
  else if (match(args[0], "time"))
    GB_time(Playernum, Governor);
  else if (match(args[0], "schedule"))
    GB_schedule(Playernum, Governor);
#ifdef VOTING
  else if (match(args[0], "vote"))
    vote(Playernum, Governor, 0);
#endif
  else if (match(args[0], "zoom"))
    zoom(Playernum, Governor, 0);
  else {
    sprintf(buf, "'%s':illegal command error.\n", args[0]);
    notify(Playernum, Governor, buf);
  }

  /* compute the prompt and send to the player */
  do_prompt(Playernum, Governor);
  sprintf(buf, "%s", Dir[Playernum - 1][Governor].prompt);
  notify(Playernum, Governor, buf);
}

void load_race_data(void) {
  int i;
  Num_races = Numraces();
  for (i = 1; i <= Num_races; i++) {
    getrace(&races[i - 1], i); /* allocates into memory */
    if (races[i - 1]->Playernum != i) {
      races[i - 1]->Playernum = i;
      putrace(races[i - 1]);
    }
  }
}

void load_star_data(void) {
  int s, t, i, j;
  startype *star_arena;
  planettype *planet_arena;
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

  planet_arena = (planettype *)malloc(pcount * sizeof(planettype));

  for (s = 0; s < Sdata.numstars; s++) {
    for (t = 0; t < Stars[s]->numplanets; t++) {
      planets[s][t] = &planet_arena[--pcount];
      getplanet(&planets[s][t], s, t);
      if (planets[s][t]->type != TYPE_ASTEROID)
        Planet_count++;
    }
  }
  /* initialize zoom factors */
  for (i = 1; i <= Num_races; i++)
    for (j = 0; j <= MAXGOVERNORS; j++) {
      Dir[i - 1][j].zoom[0] = 1.0;
      Dir[i - 1][j].zoom[1] = 0.5;
      Dir[i - 1][j].lastx[0] = Dir[i - 1][j].lastx[1] = 0.0;
      Dir[i - 1][j].lasty[0] = Dir[i - 1][j].lasty[1] = 0.0;
    }
}

void GB_time(int Playernum, int Governor) /* report back the update status */
{
  clk = time(0);
  notify(Playernum, Governor, start_buf);
  notify(Playernum, Governor, update_buf);
  notify(Playernum, Governor, segment_buf);
  sprintf(buf, "Current time    : %s", ctime(&clk));
  notify(Playernum, Governor, buf);
}

void GB_schedule(int Playernum, int Governor) {
  clk = time(0);
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

static void help(struct descriptor_data *e) {

  FILE *f;
  char file[1024];
  char *p;

  if (argn == 1) {
    help_user(e);
  } else {
    sprintf(file, "%s/%s.doc", DOCSDIR, args[1]);
    if ((f = fopen(file, "r")) != 0) {
      while (fgets(buf, sizeof buf, f)) {
        for (p = buf; *p; p++)
          if (*p == '\n') {
            *p = '\0';
            break;
          }
        strcat(buf, "\n");
        queue_string(e, buf);
      }
      fclose(f);
      queue_string(e, "----\nFinished.\n");
    } else
      queue_string(e, "Help on that subject unavailable.\n");
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
  planettype *planet;
  racetype *killer, *victim;
  shiptype *s;
  int sh;

  ship->special.mind.who_killed = Playernum;
  ship->alive = 0;
  ship->notified = 0; /* prepare the ship for recycling */

  if (ship->type != STYPE_POD && ship->type != OTYPE_FACTORY) {
    /* pods don't do things to morale, ditto for factories */
    victim = races[ship->owner - 1];
    if (victim->Gov_ship == ship->number)
      victim->Gov_ship = 0;
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

  if (ship->type == OTYPE_TOXWC && ship->whatorbits == LEVEL_PLAN) {
    getplanet(&planet, (int)ship->storbits, (int)ship->pnumorbits);
    planet->conditions[TOXIC] =
        MIN(100, planet->conditions[TOXIC] + ship->special.waste.toxic);
    putplanet(planet, (int)ship->storbits, (int)ship->pnumorbits);
  }

  /* undock the stuff docked with it */
  if (ship->docked && ship->whatorbits != LEVEL_SHIP &&
      ship->whatdest == LEVEL_SHIP) {
    (void)getship(&s, (int)ship->destshipno);
    s->docked = 0;
    s->whatdest = LEVEL_UNIV;
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

void compute_power_blocks(void) {
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
  s->whatorbits = LEVEL_UNIV;
}

void insert_sh_star(startype *star, shiptype *s) {
  s->nextship = star->ships;
  star->ships = s->number;
  s->whatorbits = LEVEL_STAR;
}

void insert_sh_plan(planettype *pl, shiptype *s) {
  s->nextship = pl->ships;
  pl->ships = s->number;
  s->whatorbits = LEVEL_PLAN;
}

void insert_sh_ship(shiptype *s, shiptype *s2) {
  s->nextship = s2->ships;
  s2->ships = s->number;
  s->whatorbits = LEVEL_SHIP;
  s->whatdest = LEVEL_SHIP;
  s->destshipno = s2->number;
}

void remove_sh_star(shiptype *s) {
  int sh;
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
      if (sh != s->number)
        free(s2);
    }
    s2->nextship = s->nextship;
    putship(s2);
    free(s2);
  }
  s->whatorbits = LEVEL_UNIV;
  s->nextship = 0;
}

void remove_sh_plan(shiptype *s) {
  int sh;
  shiptype *s2;
  planettype *p;

  getplanet(&p, (int)s->storbits, (int)s->pnumorbits);
  sh = p->ships;

  if (sh == s->number) {
    p->ships = s->nextship;
    putplanet(p, (int)s->storbits, (int)s->pnumorbits);
  } else {
    while (sh != s->number) {
      (void)getship(&s2, sh);
      sh = s2->nextship;
      if (sh != s->number)
        free(s2); /* don't free it if it is the s2 we want */
    }
    s2->nextship = s->nextship;
    putship(s2);
    free(s2);
  }
  free(p);
  s->nextship = 0;
  s->whatorbits = LEVEL_UNIV;
}

void remove_sh_ship(shiptype *s, shiptype *ship) {
  int sh;
  shiptype *s2;
  sh = ship->ships;

  if (sh == s->number)
    ship->ships = s->nextship;
  else {
    while (sh != s->number) {
      (void)getship(&s2, sh);
      sh = (int)(s2->nextship);
      if (sh != s->number)
        free(s2);
    }
    s2->nextship = s->nextship;
    putship(s2);
    free(s2);
  }
  s->nextship = 0;
  s->whatorbits = LEVEL_UNIV; /* put in limbo - wait for insert_sh.. */
}

double GetComplexity(int ship) {
  shiptype s;

  s.armor = Shipdata[ship][ABIL_ARMOR];
  s.guns = Shipdata[ship][ABIL_PRIMARY] ? PRIMARY : NONE;
  s.primary = Shipdata[ship][ABIL_GUNS];
  s.primtype = Shipdata[ship][ABIL_PRIMARY];
  s.secondary = Shipdata[ship][ABIL_GUNS];
  s.sectype = Shipdata[ship][ABIL_SECONDARY] ? SECONDARY : NONE;
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

  for (i = 0; i < NUMSTYPES; i++)
    ShipVector[i] = i;
  qsort(ShipVector, NUMSTYPES, sizeof(int), ShipCompare);
}

void warn_race(int who, char *message) {
  int i;

  for (i = 0; i <= MAXGOVERNORS; i++)
    if (races[who - 1]->governor[i].active)
      warn(who, i, message);
}

void warn(int who, int governor, char *message) {
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
  struct descriptor_data *d;
  racetype *Race;

#ifdef MONITOR
  Race = races[0]; /* deity */
  if (Race->monitor || (a != 1 && b != 1))
    notify_race(1, message);
#endif
  for (d = descriptor_list; d; d = d->next)
    if (d->connected && (d->Playernum != a || d->Governor != g) &&
        isset(Stars[star]->inhabited, d->Playernum)) {
      queue_string(d, message);
    }
}

void post_star(char *message, int star, int news) {
  int i;

  for (i = 1; i <= Num_races; i++)
    if (isset(Stars[star]->inhabited, i))
      push_telegram_race(i, message);
}

void adjust_morale(racetype *winner, racetype *loser, int amount) {
  winner->morale += amount;
  loser->morale -= amount;
  winner->points[loser->Playernum] += amount;
}
