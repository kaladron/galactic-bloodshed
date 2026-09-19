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

std::unique_ptr<Ship> ShipFactory::create(ship_struct data) {
  switch (data.type) {
    case ShipType::OTYPE_VN:
      return std::make_unique<VonNeumannShip>(std::move(data));
    case ShipType::OTYPE_BERS:
      return std::make_unique<BerserkerShip>(std::move(data));
    case ShipType::STYPE_MIRROR:
    case ShipType::OTYPE_STELE:
    case ShipType::OTYPE_GTELE:
    case ShipType::OTYPE_TRACT:
      return std::make_unique<SpaceMirrorShip>(std::move(data));
    case ShipType::STYPE_POD:
      return std::make_unique<SporePodShip>(std::move(data));
    case ShipType::OTYPE_CANIST:
    case ShipType::OTYPE_GREEN:
      return std::make_unique<CanisterShip>(std::move(data));
    case ShipType::STYPE_MISSILE:
      return std::make_unique<MissileShip>(std::move(data));
    case ShipType::STYPE_MINE:
      return std::make_unique<MineShip>(std::move(data));
    case ShipType::OTYPE_TERRA:
      return std::make_unique<TerraformerShip>(std::move(data));
    case ShipType::OTYPE_PLOW:
      return std::make_unique<GroundPlowShip>(std::move(data));
    case ShipType::OTYPE_TRANSDEV:
      return std::make_unique<TransporterShip>(std::move(data));
    case ShipType::OTYPE_TOXWC:
      return std::make_unique<ToxicWasteShip>(std::move(data));
    default:
      return std::make_unique<Ship>(std::move(data));
  }
}

std::unique_ptr<Ship> ShipFactory::create_from_template(ShipType type,
                                                        player_t owner) {
  const auto& tmpl = ship_template(type);
  ship_struct data{
      .owner = owner,
      .name = std::string(tmpl.name),
      .armor = tmpl.base_armor,
      .max_crew = tmpl.max_crew,
      .max_resource = tmpl.max_resource,
      .max_destruct = tmpl.max_destruct,
      .max_fuel = tmpl.max_fuel,
      .max_speed = tmpl.base_speed,
      .build_type = type,
      .build_cost = tmpl.build_cost,
      .retaliate = tmpl.max_guns,
      .type = type,
      .active = true,
      .alive = true,
      .guns = tmpl.has_primary() ? ActiveBattery::PRIMARY : ActiveBattery::NONE,
      .primary_battery =
          GunBattery::create(tmpl.max_guns, shipdata_primary(type)),
      .secondary_battery = GunBattery::create(0, shipdata_secondary(type)),
      .max_hanger = tmpl.max_hangar,
  };
  if (type == ShipType::OTYPE_VN || type == ShipType::OTYPE_BERS) {
    data.special = MindData{.progenitor = owner};
  }

  auto ship = create(std::move(data));
  ship->size() = ship->calculate_size();
  ship->set_mass(ship->base_mass());
  ship->build_cost() = cost(*ship);
  return ship;
}
