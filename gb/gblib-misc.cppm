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

import :gameobj;
import :planet;
import :race;
import :ships;
import :star;
import :tweakables;
import :universe;
import :types;

import std.compat;

export class DescriptorData : public GameObj {
public:
  DescriptorData(int sock, EntityManager& em) : GameObj{em} {
    // TODO(jeffbailey): Pull the fprintf stuff out of this constructor
    struct sockaddr_in6 addr;
    socklen_t addr_len = sizeof(addr);

    descriptor = accept(sock, (struct sockaddr*)&addr, &addr_len);
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
  std::string raw_input;
  time_t last_time{};
  int quota{};
  bool operator==(const DescriptorData& rhs) const noexcept {
    return descriptor == rhs.descriptor && player == rhs.player &&
           governor == rhs.governor;
  }
};

export void notify_race(player_t, const std::string&);
export bool notify(player_t, governor_t, const std::string&);
export void d_think(EntityManager&, player_t, governor_t, const std::string&);
export void d_broadcast(EntityManager&, player_t, governor_t,
                        const std::string&);
export void d_shout(player_t, governor_t, const std::string&);
export void d_announce(EntityManager&, player_t, governor_t, starnum_t,
                       const std::string&);
// New signature using EntityManager
export void warn_race(EntityManager&, player_t, const std::string&);
export void warn(player_t, governor_t, const std::string&);
// New signature using EntityManager
export void warn_star(EntityManager&, player_t, starnum_t, const std::string&);
// New signature using EntityManager
export void notify_star(EntityManager&, player_t, governor_t, starnum_t,
                        const std::string&);
export void adjust_morale(Race&, Race&, int);

export void queue_string(DescriptorData&, const std::string&);
export void add_to_queue(std::deque<std::string>&, const std::string&);
export void strstr_to_queue(DescriptorData&);

// Diagnostic logging for invariant violations
export constexpr bool kDebugInvariants = true;

export template <typename T, typename U>
void log_invariant_violation(
    std::string_view entity, std::string_view field, T attempted, U clamped_to,
    std::source_location loc = std::source_location::current()) {
  if constexpr (kDebugInvariants) {
    std::print(
        stderr, "[INVARIANT] {}::{}: attempted {}, clamped to {} (at {}:{})\n",
        entity, field, attempted, clamped_to, loc.file_name(), loc.line());
  }
}

export template <typename T>
concept Unsigned = std::is_unsigned_v<T>;

export template <typename T>
void setbit(T& target, const Unsigned auto pos)
  requires Unsigned<T>
{
  T bit = 1;
  target |= (bit << pos);
}

export template <typename T>
void clrbit(T& target, const Unsigned auto pos)
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

export template <typename T, typename U>
constexpr auto MIN(const T& x, const U& y) {
  return (x < y) ? x : y;
}

export template <typename T, typename U>
constexpr auto MAX(const T& x, const U& y) {
  return (x > y) ? x : y;
}

export double tech_prod(const money_t investment, const population_t popn) {
  double scale = static_cast<double>(popn) / 10000.;
  return (TECH_INVEST *
          std::log10(static_cast<double>(investment) * scale + 1.0));
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

/**
 * \brief Scales used in production efficiency etc.
 * \param x Integer from 0-100
 * \return Float 0.0 - 1.0 (logscaleOB 0.5 - .95)
 */
export constexpr double logscale(const int x) {
  return log10((double)x + 1.0) / 2.0;
}

/**
 * @brief Formats a numeric value as an estimated string with K/M suffixes.
 *
 * Provides translated estimates of numeric values based on the observer's
 * translation capability. Values are rounded based on translation level
 * and formatted with K (thousands) or M (millions) suffixes for readability.
 *
 * @tparam T Arithmetic type (int, double, float, etc.)
 * @param data The numeric value to estimate
 * @param r The observing race
 * @param p The player number being observed
 * @return Formatted string with K/M suffix, or "?" if translation too low
 */
export template <typename T>
  requires std::is_arithmetic_v<T>
std::string estimate(const T data, const Race& r, const player_t p) {
  if (r.translate[p - 1] > 10) {
    int k = 101 - std::min(r.translate[p - 1], 100);
    int est = (std::abs(static_cast<int>(data)) / k) * k;
    if (est < 1000) return std::format("{}", est);
    if (est < 10000) {
      return std::format("{:.1f}K", static_cast<double>(est) / 1000.);
    }
    if (est < 1000000) {
      return std::format("{:.0f}K", static_cast<double>(est) / 1000.);
    }

    return std::format("{:.1f}M", static_cast<double>(est) / 1000000.);
  }
  return "?";
}

export void insert_sh_univ(universe_struct*, Ship*);
export void insert_sh_star(Star&, Ship*);
export void insert_sh_plan(Planet&, Ship*);
export void insert_sh_ship(Ship*, Ship*);
export void remove_sh_star(EntityManager&, Ship&);
export void remove_sh_plan(EntityManager&, Ship&);
export void remove_sh_ship(EntityManager&, Ship&, Ship&);