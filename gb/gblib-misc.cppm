// SPDX-License-Identifier: Apache-2.0

module;

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cerrno>
#include <cstdio>

export module gblib:misc;

import :race;
import :tweakables;
import :types;
import std.compat;

#include "gb/tweakables.h"

export class DescriptorData : public GameObj {
 public:
  DescriptorData(int sock, Db &db_) : GameObj{db_} {
    // TODO(jeffbailey): Pull the fprintf stuff out of this constructor
    struct sockaddr_in6 addr;
    socklen_t addr_len = sizeof(addr);

    descriptor = accept(sock, (struct sockaddr *)&addr, &addr_len);
    // TODO(jeffbailey): The original code didn't error on EINTR or EMFILE, but
    // also didn't halt processing on an invalid socket.  Need to evaluate the
    // cases we should handle here properly.
    if (descriptor <= 0) throw std::runtime_error(std::string{strerror(errno)});

    char addrstr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &addr.sin6_addr, addrstr, sizeof(addrstr));
    // TODO(jeffbailey): There used to be custom access check stuff here.
    // It should be replaced with TCP wrappers or something similar
    fprintf(stderr, "ACCEPT from %s(%d) on descriptor %d\n", addrstr,
            ntohs(addr.sin6_port), descriptor);
  }
  int descriptor;
  bool connected{};
  ssize_t output_size{};
  std::deque<std::string> output;
  std::deque<std::string> input;
  char *raw_input{};
  char *raw_input_at{};
  time_t last_time{};
  int quota{};
  bool operator==(const DescriptorData &rhs) const noexcept {
    return descriptor == rhs.descriptor && player == rhs.player &&
           governor == rhs.governor;
  }
};

export void notify_race(const player_t, const std::string &);
export bool notify(const player_t, const governor_t, const std::string &);
export void d_think(const player_t, const governor_t, const std::string &);
export void d_broadcast(const player_t, const governor_t, const std::string &);
export void d_shout(const player_t, const governor_t, const std::string &);
export void d_announce(const player_t, const governor_t, const starnum_t,
                       const std::string &);
export void warn_race(const player_t, const std::string &);
export void warn(const player_t, const governor_t, const std::string &);
export void warn_star(const player_t, const starnum_t, const std::string &);
export void notify_star(const player_t, const governor_t, const starnum_t,
                        const std::string &);
export void adjust_morale(Race &, Race &, int);

export void queue_string(DescriptorData &, const std::string &);
export void add_to_queue(std::deque<std::string> &, const std::string &);
export void strstr_to_queue(DescriptorData &);
