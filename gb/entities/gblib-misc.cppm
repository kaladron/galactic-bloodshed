// SPDX-License-Identifier: Apache-2.0

module;

export module gblib:misc;

import gb.entities;
import gb.services;

import strong_id;
import std;

// Note: Notification functions moved to gb/services/notification.{cppm,cc}
// - d_broadcast, d_announce, d_think, d_shout
// - warn_player, warn_race, notify_star, warn_star
// notify_race and notify_player are now methods on SessionRegistry

export void adjust_morale(Race&, Race&, int);

export void add_to_queue(std::deque<std::string>&, const std::string&);

// Helper for turn processing: send telegram to all inhabitants of a star
export void telegram_star(EntityManager&, starnum_t, player_t sender,
                          governor_t sender_gov, const std::string& message);

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