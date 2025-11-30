// SPDX-License-Identifier: Apache-2.0

/// \file gblib-shipfilter.cppm
/// \brief Ship filtering helpers for ShipList iteration

export module gblib:shipfilter;

import :ships;
import :types;
import std;

namespace GB {

/**
 * \brief Parse ship selection string to determine starting ship number
 *
 * Supports:
 * - "#123" or "123" - Returns ship number 123
 * - "f", "frd", "*" - Returns nullopt (caller should use scope-based iteration)
 * - "" - Returns nullopt
 *
 * \param selection User-provided selection string
 * \return Ship number if a specific ship was selected, nullopt otherwise
 */
export std::optional<shipnum_t>
parse_ship_selection(std::string_view selection) {
  // Strip leading '#' characters
  while (selection.size() > 1 && selection.front() == '#') {
    selection.remove_prefix(1);
  }

  // Check if it's a number
  if (selection.size() > 0 && std::isdigit(selection.front())) {
    return std::stoi(std::string(selection.begin(), selection.end()));
  }

  return std::nullopt;
}

/**
 * \brief Check if a ship matches the filter string
 *
 * Supports multiple filter types:
 * - "#123" - Match specific ship number (MUST have # prefix)
 * - "f" - Match ship type (single character from Shipltrs)
 * - "frd" or "123" - Match any of multiple ship types (numeric strings are
 * treated as type letters, not ship numbers)
 * - "*" - Match all ships (wildcard)
 *
 * \param filter User-provided filter string
 * \param ship Ship to check against filter
 * \return true if ship matches the filter
 */
export bool ship_matches_filter(std::string_view filter, const Ship& ship) {
  // Empty filter matches nothing
  if (filter.empty()) return false;

  // If filter starts with '#', it's a ship number filter
  if (filter[0] == '#') {
    auto shipnum = parse_ship_selection(filter);
    if (shipnum) {
      return ship.number() == *shipnum;
    }
    return false;
  }

  // Otherwise, treat as ship type filter (match any character in filter string)
  // This includes numeric strings like "123" which look for ship types with
  // letters '1', '2', '3'
  for (const auto& c : filter) {
    if (c == ::Shipltrs[ship.type()] || c == '*') {
      return true;
    }
  }

  return false;
}

/**
 * \brief Check if the filter is for a specific ship number
 *
 * \param filter User-provided filter string
 * \return true if filter specifies a ship number (e.g., "#123")
 */
export bool is_ship_number_filter(std::string_view filter) {
  if (filter.empty()) return false;
  return filter[0] == '#';
}

}  // namespace GB
