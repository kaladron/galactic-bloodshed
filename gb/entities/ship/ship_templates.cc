// SPDX-License-Identifier: Apache-2.0

/// \file ship_templates.cc
/// \brief Ship template complexity calculation.

module;

import std;

module gblib;

/**
 * @brief Calculate the complexity (tech level) for a default ship of a type.
 *
 * For a ship with no modifications, this returns exactly its base tech
 * requirement. This is useful for sorting ship types by their base complexity.
 *
 * @param type The ShipType to get default complexity for.
 * @return The base complexity value for this ship type.
 */
double complexity(ShipType type) {
  // For an unmodified ship, complexity() returns exactly the base tech.
  // We can compute this directly without creating a full Ship object.
  return ship_template(type).base_tech;
}
