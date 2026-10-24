// SPDX-License-Identifier: Apache-2.0

/// \file navigation.cppm
/// \brief Tier-6a Presentation submodule for navigation, trip estimates,
/// ship capture reports, and ship orders.

export module gb.presentation.navigation;

export import gb.presentation.core;

import gb.entities;
import gb.services;
import gb.mechanics;
import std;

export namespace GB::presentation {

/// Formats a `TripEstimate` into its ASCII presentation string.
[[nodiscard]] std::string render_trip_estimate(const TripEstimate& est);

/// Formats a `CapturedShipsReport` into its ASCII presentation string.
[[nodiscard]] std::string
render_captured_ships_report(const CapturedShipsReport& report);

/// Formats an `OrderError` into its user-facing error diagnostic string.
[[nodiscard]] std::string format_order_error(const OrderError& err);

/// Formats an `OrderUpdate` notice into its ASCII presentation string.
[[nodiscard]] std::string render_order_update(const OrderUpdate& update);

/// Returns the ASCII table header for standing ship orders.
[[nodiscard]] std::string
render_ship_orders_header(const ShipOrdersHeader& header);

/// Formats a `ShipOrderStatus` into its ASCII presentation string.
[[nodiscard]] std::string
render_ship_order_status(const ShipOrderStatus& status);

/// Open ADL dispatch for `TripEstimate`.
[[nodiscard]] std::string render_for_mode(UiMode mode, const TripEstimate& vm);

/// Open ADL dispatch for `CapturedShipsReport`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const CapturedShipsReport& vm);

/// Open ADL dispatch for `ShipOrdersHeader`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ShipOrdersHeader& vm);

/// Open ADL dispatch for `ShipOrderStatus`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ShipOrderStatus& vm);

/// Open ADL dispatch for `OrderUpdate`.
[[nodiscard]] std::string render_for_mode(UiMode mode, const OrderUpdate& vm);

/// View model for `fuel` (`proj_fuel`) command reports.
struct ProjFuelViewModel {
  double current_fuel_cargo{0.0};
  std::optional<TripEstimate> current_estimate{std::nullopt};
  double optimum_fuel_level{0.0};
  std::optional<TripEstimate> optimum_estimate{std::nullopt};
};

/// Formats a `ProjFuelViewModel` into its ASCII presentation string.
[[nodiscard]] std::string render_proj_fuel(const ProjFuelViewModel& vm);

/// Serializes a `ProjFuelViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"fuel","data":{...}}\n`).
[[nodiscard]] std::string render_json_proj_fuel(const ProjFuelViewModel& vm);

/// Open ADL dispatch for `ProjFuelViewModel`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ProjFuelViewModel& vm);

/// Formats an `AlienSectorAssaultResult` into its caller-facing ASCII report.
[[nodiscard]] std::string
render_alien_sector_assault(const AlienSectorAssaultResult& assault);

/// Formats the optional Metamorph body-absorption telegram to the defender.
[[nodiscard]] std::optional<std::string>
render_alien_assault_absorption_telegram(
    const AlienSectorAssaultResult& assault);

/// Formats the defender's ground-assault summary telegram.
[[nodiscard]] std::string
render_alien_assault_defender_telegram(const AlienSectorAssaultResult& assault);

/// Formats the galactic combat news bulletin for an alien sector assault.
[[nodiscard]] std::string
render_alien_assault_news(const AlienSectorAssaultResult& assault);

/// Formats a `PlanetCargoTransferResult` into its ASCII presentation string.
[[nodiscard]] std::string
render_planet_cargo_transfer(const PlanetCargoTransferResult& res);

/// Serializes a `PlanetCargoTransferResult` into a newline-delimited Glaze JSON
/// envelope (`{"type":"planet_cargo_transfer","data":{...}}\n`).
[[nodiscard]] std::string
render_json_planet_cargo_transfer(const PlanetCargoTransferResult& res);

/// Open ADL dispatch for `PlanetCargoTransferResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const PlanetCargoTransferResult& vm);

/// Formats a `ShipDockingError` into its user-facing error diagnostic string.
[[nodiscard]] std::string
format_ship_docking_error(const ShipDockingError& err);

/// Formats a `ShipDockingStatus` into its ASCII presentation string.
[[nodiscard]] std::string
render_ship_docking_status(const ShipDockingStatus& status);

/// Serializes a `ShipDockingStatus` into a newline-delimited Glaze JSON
/// envelope (`{"type":"ship_docking_status","data":{...}}\n`).
[[nodiscard]] std::string
render_json_ship_docking_status(const ShipDockingStatus& status);

/// Open ADL dispatch for `ShipDockingStatus`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ShipDockingStatus& vm);

/// Formats a `ShipCargoTransferResult` into its ASCII presentation string.
[[nodiscard]] std::string
render_ship_cargo_transfer(const ShipCargoTransferResult& res);

/// Serializes a `ShipCargoTransferResult` into a newline-delimited Glaze JSON
/// envelope (`{"type":"ship_cargo_transfer","data":{...}}\n`).
[[nodiscard]] std::string
render_json_ship_cargo_transfer(const ShipCargoTransferResult& res);

/// Open ADL dispatch for `ShipCargoTransferResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ShipCargoTransferResult& vm);

/// Formats the optional cross-player telegram when unloading cargo to another
/// player's docked ship.
[[nodiscard]] std::optional<std::string>
render_ship_cargo_transfer_telegram(const ShipCargoTransferResult& res);

/// Formats a `TransporterError` into its user-facing error diagnostic string.
[[nodiscard]] std::string format_transporter_error(TransporterError err);

/// Formats a `TransporterTransferResult` into its ASCII presentation string.
[[nodiscard]] std::string
render_transporter_transfer(const TransporterTransferResult& res);

/// Serializes a `TransporterTransferResult` into a newline-delimited Glaze JSON
/// envelope (`{"type":"transporter_transfer","data":{...}}\n`).
[[nodiscard]] std::string
render_json_transporter_transfer(const TransporterTransferResult& res);

/// Open ADL dispatch for `TransporterTransferResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const TransporterTransferResult& vm);

/// Formats the optional cross-player telegram when beaming cargo via a
/// transporter device to another player's receiver.
[[nodiscard]] std::optional<std::string>
render_transporter_telegram(const TransporterTransferResult& res);

/// Formats a `LaunchError` into its user-facing error diagnostic string.
[[nodiscard]] std::string format_launch_error(const LaunchError& err);

/// Formats a `LaunchShipResult` into its ASCII presentation string.
[[nodiscard]] std::string
render_launch_ship_result(const LaunchShipResult& res);

/// Serializes a `LaunchShipResult` into a newline-delimited Glaze JSON
/// envelope (`{"type":"launch_ship","data":{...}}\n`).
[[nodiscard]] std::string
render_json_launch_ship_result(const LaunchShipResult& res);

/// Open ADL dispatch for `LaunchShipResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const LaunchShipResult& vm);

/// Formats the observer telegram sent to foreign races on a planet when a ship
/// launches into orbit.
[[nodiscard]] std::string
render_launch_observed_telegram(const LaunchShipResult& res);

/// Formats a `LandError` into its user-facing error diagnostic string.
[[nodiscard]] std::string format_land_error(const LandError& err);

/// Formats a `LandShipResult` into its ASCII presentation string.
[[nodiscard]] std::string render_land_ship_result(const LandShipResult& res);

/// Serializes a `LandShipResult` into a newline-delimited Glaze JSON
/// envelope (`{"type":"land_ship","data":{...}}\n`).
[[nodiscard]] std::string
render_json_land_ship_result(const LandShipResult& res);

/// Open ADL dispatch for `LandShipResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const LandShipResult& vm);

/// Formats the optional observer or crash telegram sent to races on a planet
/// when a ship lands or crashes.
[[nodiscard]] std::optional<std::string>
render_land_observer_telegram(const LandShipResult& res);

/// Result of changing the active session scope (`cs` / `cs <scope>`).
struct ScopeChangeResult {
  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  starnum_t snum{0};
  planetnum_t pnum{0};
  shipnum_t shipno{0};
  std::string path{};
  UniverseCoordinates universe_center{0.0, 0.0};
  SystemCoordinates system_center{0.0, 0.0};
};

/// Result of updating a governor's persistent default home scope
/// (`cs -d <scope>`).
struct DefaultScopeResult {
  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  starnum_t snum{0};
  planetnum_t pnum{0};
  std::string path{};
};

[[nodiscard]] std::string render_scope_change(const ScopeChangeResult& vm);
[[nodiscard]] std::string render_json_scope_change(const ScopeChangeResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ScopeChangeResult& vm);

[[nodiscard]] std::string
render_default_scope_result(const DefaultScopeResult& vm);
[[nodiscard]] std::string
render_json_default_scope_result(const DefaultScopeResult& vm);
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const DefaultScopeResult& vm);

}  // namespace GB::presentation
