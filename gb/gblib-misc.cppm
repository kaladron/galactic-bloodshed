// SPDX-License-Identifier: Apache-2.0

module;

export module gblib:misc;

import :gameobj;
import :planet;
import :race;
import :ships;
import :star;
import :tweakables;
import :universe;
import :types;

import strong_id;
import std.compat;

// Note: Notification functions moved to gb/services/notification.{cppm,cc}
// - d_broadcast, d_announce, d_think, d_shout
// - warn_player, warn_race, notify_star, warn_star
// notify_race and notify_player are now methods on SessionRegistry

export void adjust_morale(Race&, Race&, int);

export void add_to_queue(std::deque<std::string>&, const std::string&);

// Helper for turn processing: send telegram to all inhabitants of a star
export void telegram_star(EntityManager&, starnum_t, player_t sender,
                          governor_t sender_gov, const std::string& message);

// Diagnostic logging for invariant violations
export constexpr bool kDebugInvariants = true;

export template <typename T, typename U>
void log_invariant_violation(
    std::string_view entity, std::string_view field, T attempted, U clamped_to,
    std::source_location loc = std::source_location::current()) {
  if constexpr (kDebugInvariants) {
    std::print(std::cerr,
               "[INVARIANT] {}::{}: attempted {}, clamped to {} (at {}:{})\n",
               entity, field, attempted, clamped_to, loc.file_name(),
               loc.line());
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

// Overloads for ID types (player_t, governor_t, etc.)
// These handle the .value extraction and cast to unsigned automatically
export template <typename T, FixedString Tag, typename IDValueType>
void setbit(T& target, const ID<Tag, IDValueType> id)
  requires Unsigned<T> && std::integral<IDValueType>
{
  setbit(target, static_cast<unsigned>(id.value));
}

export template <typename T, FixedString Tag, typename IDValueType>
void clrbit(T& target, const ID<Tag, IDValueType> id)
  requires Unsigned<T> && std::integral<IDValueType>
{
  clrbit(target, static_cast<unsigned>(id.value));
}

export template <typename T, FixedString Tag, typename IDValueType>
bool isset(const T target, const ID<Tag, IDValueType> id)
  requires Unsigned<T> && std::integral<IDValueType>
{
  return isset(target, static_cast<unsigned>(id.value));
}

export template <typename T, FixedString Tag, typename IDValueType>
bool isclr(const T target, const ID<Tag, IDValueType> id)
  requires Unsigned<T> && std::integral<IDValueType>
{
  return isclr(target, static_cast<unsigned>(id.value));
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
  if (r.translate[p.value - 1] > 10) {
    int k = 101 - std::min(r.translate[p.value - 1], 100);
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