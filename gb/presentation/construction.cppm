// SPDX-License-Identifier: Apache-2.0

/// \file construction.cppm
/// \brief Tier-6a Presentation submodule for ship construction errors and
/// summaries.

export module gb.presentation.construction;

export import gb.presentation.core;

import gb.entities;
import gb.services;
import gb.mechanics;
import std;

export namespace GB::presentation {

/// Formats a `PlanetBuildError` into its user-facing error diagnostic string.
[[nodiscard]] std::string
format_planet_build_error(const PlanetBuildError& err);

/// Formats a `ShipBuildError` into its user-facing error diagnostic string.
[[nodiscard]] std::string format_ship_build_error(ShipBuildError err);

/// Formats a `CreatedShipSummary` into its ASCII presentation string.
[[nodiscard]] std::string
render_created_ship_summary(const CreatedShipSummary& summary);

/// Formats an `InitializedShipReport` into its ASCII presentation string.
[[nodiscard]] std::string
render_initialized_ship_report(const InitializedShipReport& report);

/// Open ADL dispatch for `CreatedShipSummary`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const CreatedShipSummary& vm);

/// Open ADL dispatch for `InitializedShipReport`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const InitializedShipReport& vm);

}  // namespace GB::presentation
