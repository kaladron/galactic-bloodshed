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

}  // namespace GB::presentation
