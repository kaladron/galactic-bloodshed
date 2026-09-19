// SPDX-License-Identifier: Apache-2.0

/// \file shlmisc.cc
/// \brief Miscellaneous stuff included in the shell.

module;

import std;

module gblib;

bool authorized(const governor_t Governor, const Ship& ship) {
  return ship.is_authorized_for(Governor);
}

/**
 * \brief Check is the ship is in the given input string.
 *
 * See start_shiplist's comment for more details.
 */
bool in_list(const player_t playernum, const std::string_view list,
             const Ship& s, shipnum_t* nextshipno) {
  if (s.owner() != playernum || !s.alive()) return false;

  if (list.length() == 0) return false;

  if (list[0] == '#' || std::isdigit(list[0])) {
    *nextshipno = 0;
    return true;
  }

  // Match either the ship letter or * for wildcard.
  for (const auto& p : list)
    if (p == s.type_letter() || p == '*') return true;
  return false;
}

std::optional<std::tuple<int, int, int, int>> get4args(std::string_view s) {
  if (s.empty()) return std::nullopt;

  // Find the comma separating x and y parts
  auto comma_pos = s.find(',');
  if (comma_pos == std::string_view::npos) return std::nullopt;

  std::string_view x_part = s.substr(0, comma_pos);
  std::string_view y_part = s.substr(comma_pos + 1);

  int xl = 0;
  int xh = 0;
  int yl = 0;
  int yh = 0;

  // Parse x coordinates (either "x" or "xl:xh")
  auto x_colon = x_part.find(':');
  if (x_colon != std::string_view::npos) {
    // Range format: xl:xh
    auto xl_result =
        std::from_chars(x_part.data(), x_part.data() + x_colon, xl);
    if (xl_result.ec != std::errc{} ||
        xl_result.ptr != x_part.data() + x_colon) {
      return std::nullopt;
    }

    auto xh_part = x_part.substr(x_colon + 1);
    auto xh_result =
        std::from_chars(xh_part.data(), xh_part.data() + xh_part.size(), xh);
    if (xh_result.ec != std::errc{} ||
        xh_result.ptr != xh_part.data() + xh_part.size()) {
      return std::nullopt;
    }
  } else {
    // Single value format: x
    auto x_result =
        std::from_chars(x_part.data(), x_part.data() + x_part.size(), xl);
    if (x_result.ec != std::errc{} ||
        x_result.ptr != x_part.data() + x_part.size()) {
      return std::nullopt;
    }
    xh = xl;
  }

  // Parse y coordinates (either "y" or "yl:yh")
  auto y_colon = y_part.find(':');
  if (y_colon != std::string_view::npos) {
    // Range format: yl:yh
    auto yl_result =
        std::from_chars(y_part.data(), y_part.data() + y_colon, yl);
    if (yl_result.ec != std::errc{} || yl_result.ptr != y_part.data() + y_colon)
      return std::nullopt;

    auto yh_part = y_part.substr(y_colon + 1);
    auto yh_result =
        std::from_chars(yh_part.data(), yh_part.data() + yh_part.size(), yh);
    if (yh_result.ec != std::errc{} ||
        yh_result.ptr != yh_part.data() + yh_part.size())
      return std::nullopt;
  } else {
    // Single value format: y
    auto y_result =
        std::from_chars(y_part.data(), y_part.data() + y_part.size(), yl);
    if (y_result.ec != std::errc{} ||
        y_result.ptr != y_part.data() + y_part.size())
      return std::nullopt;
    yh = yl;
  }

  return std::make_tuple(xl, xh, yl, yh);
}
