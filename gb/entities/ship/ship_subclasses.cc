// SPDX-License-Identifier: Apache-2.0

/// \file ship_subclasses.cc
/// \brief Specialized Ship domain subclass methods.

module;

import std;

module gb.entities;

namespace {

constexpr double TAN_22_5_DEG = std::numbers::sqrt2 - 1.0;
constexpr double TAN_67_5_DEG = std::numbers::sqrt2 + 1.0;

[[nodiscard]] int octant_for_positive_dy(double slope) noexcept {
  if (slope < -TAN_67_5_DEG || slope > TAN_67_5_DEG) return 4;
  if (slope > TAN_22_5_DEG) return 3;
  if (slope > 0.000) return 2;
  if (slope > -TAN_22_5_DEG) return 6;
  return 5;
}

[[nodiscard]] int octant_for_negative_dy(double slope) noexcept {
  if (slope < -TAN_67_5_DEG || slope > TAN_67_5_DEG) return 0;
  if (slope > TAN_22_5_DEG) return 7;
  if (slope > 0.000) return 6;
  if (slope > -TAN_22_5_DEG) return 2;
  return 1;
}

}  // namespace

/// \brief Computes the 8-octant compass heading (0..7) toward the given
/// target coordinates.
///
/// The 8 compass directions correspond to:
/// - 0: North (0 deg)
/// - 1: North-East (45 deg)
/// - 2: East (90 deg)
/// - 3: South-East (135 deg)
/// - 4: South (180 deg)
/// - 5: South-West (225 deg)
/// - 6: West (270 deg)
/// - 7: North-West (315 deg)
///
/// The slope boundaries are based on the tangent of half-octant (22.5 deg)
/// boundaries:
/// - tan(22.5 deg) = sqrt(2) - 1 ≈ 0.4142
/// - tan(67.5 deg) = sqrt(2) + 1 ≈ 2.4142
///
/// \param target_coords Absolute universe coordinates of the target.
/// \return Compass direction heading index (0..7).
int SpaceMirrorShip::aim_direction(
    UniverseCoordinates target_coords) const noexcept {
  const auto [xt, yt] = target_coords;
  const auto my_coords = coordinates();
  if (xt == my_coords.x) {
    return (yt > my_coords.y) ? 4 : 0;
  }
  if (yt == my_coords.y) {
    return (xt > my_coords.x) ? 2 : 6;
  }

  const double slope = (yt - my_coords.y) / (xt - my_coords.x);
  return (yt > my_coords.y) ? octant_for_positive_dy(slope)
                            : octant_for_negative_dy(slope);
}
