// SPDX-License-Identifier: Apache-2.0

module;

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>

export module gblib:misc;

import :race;
import :tweakables;
import :types;

import std.compat;

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

void Fileread(int fd, char *p, size_t num, int posn) {
  if (lseek(fd, posn, L_SET) < 0) {
    perror("Fileread 1");
    return;
  }
  if ((read(fd, p, num)) != num) {
    perror("Fileread 2");
  }
}

void Filewrite(int fd, const char *p, size_t num, int posn) {
  if (lseek(fd, posn, L_SET) < 0) {
    perror("Filewrite 1");
    return;
  }

  if ((write(fd, p, num)) != num) {
    perror("Filewrite 2");
    return;
  }
}

export template <typename T>
concept Unsigned = std::is_unsigned<T>::value;

export template <typename T>
void setbit(T &target, const Unsigned auto pos)
  requires Unsigned<T>
{
  T bit = 1;
  target |= (bit << pos);
}

export template <typename T>
void clrbit(T &target, const Unsigned auto pos)
  requires Unsigned<T>
{
  T bit = 1;
  target &= ~(bit << pos);
}

export template <typename T>
bool isset(const T target, const Unsigned auto pos)
  requires Unsigned<T>
{
  T bit = 1;
  return target & (bit << pos);
}

export template <typename T>
bool isclr(const T target, const Unsigned auto pos)
  requires Unsigned<T>
{
  return !isset(target, pos);
}

export double tech_prod(const money_t investment, const population_t popn) {
  double scale = static_cast<double>(popn) / 10000.;
  return (TECH_INVEST *
          std::log10(static_cast<double>(investment) * scale + 1.0));
}

/**
 * @brief Calculates the squared distance between two points in a 2D space.
 *
 * This function calculates the squared distance between two points (x1, y1) and
 * (x2, y2) in a 2D space.
 *
 * @param x1 The x-coordinate of the first point.
 * @param y1 The y-coordinate of the first point.
 * @param x2 The x-coordinate of the second point.
 * @param y2 The y-coordinate of the second point.
 * @return The squared distance between the two points.
 */
export double Distsq(double x1, double y1, double x2, double y2) {
  return ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

/**
 * @brief Calculates the positive modulus of two integers.
 *
 * This function calculates the modulus of the given integers `a` and `b`, and
 * then returns the absolute value.
 *
 * @param a The dividend.
 * @param b The divisor.
 * @return The modulus of `a` and `b`.
 */
export int mod(int a, int b) {
  int dum = a % b;
  return std::abs(dum);
}
