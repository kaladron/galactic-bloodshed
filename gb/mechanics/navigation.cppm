// SPDX-License-Identifier: Apache-2.0

/// \file navigation.cppm
/// \brief Ship navigation, sublight/hyperdrive movement, trip simulation, and
/// ship order mechanics.

module;

export module gb.mechanics:navigation;

import gb.entities;
import gb.services;
import std;

export struct CapturedShipEvent {
  shipnum_t ship_number{0};
  std::string ship_display{};
  player_t new_owner{0};
  governor_t new_governor{0};
};

export struct CapturedShipsReport {
  std::vector<CapturedShipEvent> captured_ships{};
};

export armor_t getdefense(EntityManager&, const Ship&);
export CapturedShipsReport capture_stuff(EntityManager&, const Ship&);
export void domass(Ship&, EntityManager&);
export void doown(Ship&, EntityManager&);
export std::string prin_ship_orbits(EntityManager&, const Ship&);
export std::string format_ship_dest(EntityManager&, const Ship&);
export void moveship(EntityManager&, Ship& ship, bool is_update,
                     bool send_messages, bool checking_fuel);
export void msg_OOF(EntityManager&, const Ship& ship);
export bool followable(EntityManager&, const Ship& ship, const Ship& target);
export std::string dispshiploc_brief(EntityManager&, const Ship&);
export std::string dispshiploc(EntityManager&, const Ship&);

export std::tuple<bool, segments_t> do_trip(const Place&, SimulatedShip&,
                                            double fuel, double gravity_factor,
                                            UniverseCoordinates dest_coords,
                                            EntityManager&);

export enum class ArrivalTimeStatus {
  Available,
  ServerStateUnavailable,
  SegmentDiscrepancy,
};

export struct TripEstimate {
  double distance = 0.0;
  segments_t segments = 0;
  double fuel_used = 0.0;
  double launch_gravity_fuel = 0.0;
  std::string launch_planet_name;
  ArrivalTimeStatus arrival_status = ArrivalTimeStatus::Available;
  std::time_t estimated_arrival_time = 0;
};

export TripEstimate compute_trip_estimate(EntityManager& em, double dist,
                                          double fuel, double grav, double mass,
                                          segments_t segs,
                                          std::string_view plan_buf);

export struct ShipOrdersHeader {};

export struct ShipOrderStatus {
  shipnum_t ship_number{0};
  char type_letter{' '};
  std::string name{};
  char hyper_indicator{' '};
  speed_t speed{0};
  std::string orbits_display{};
  std::string destination_display{};
  std::string combat_options{};
  std::string navigation_options{};
  std::string specialty_options{};
  bool has_hyperdrive_jump{false};
  double jump_distance{0.0};
  double jump_fuel_cost{0.0};
  bool insufficient_fuel_capacity{false};
};

export enum class OrderErrorReason {
  ShipIrradiated,
  ShipHasNoCrew,
  CannotBeAssignedOrders,
  ShipDockedUseLaunchOrUndock,
  NoHyperDriveCapability,
  DestinationMustBeStarOrPlanet,
  CannotProtectSelf,
  CannotProtect,
  CannotBeLaunched,
  ShipDockedUndockOrLaunchFirst,
  InvalidPlace,
  TargetShipOutOfRange,
  SystemUnexplored,
  ShipTypeCannotRetaliate,
  ShipCannotRetaliate,
  NoLaser,
  NotEquippedWithCombatLasers,
  NoCrystalMounted,
  BadRouteNumber,
  NoSpeedRating,
  InvalidSpeed,
  InvalidSalvoGunCount,
  NoPrimaryGuns,
  NoSecondaryGuns,
  InvalidBatteryGunCount,
};

export struct OrderError {
  OrderErrorReason reason{OrderErrorReason::CannotBeAssignedOrders};
  std::string ship_display{};
  radiation_t radiation{0};
  std::optional<PlaceError> place_error{std::nullopt};
};

export struct OrderUpdate {
  bool modified{true};
};

export ShipOrderStatus query_ship_order(EntityManager& em, const Ship& ship);
export std::expected<OrderUpdate, OrderError>
give_orders(GameObj&, const command_t&, int, Ship&);
