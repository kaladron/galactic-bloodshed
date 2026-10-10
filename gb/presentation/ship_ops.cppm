// SPDX-License-Identifier: Apache-2.0

/// \file ship_ops.cppm
/// \brief Tier-6a Presentation submodule for single-ship operations (detonate,
/// jettison, mount, dismount, grant, scrap, peaceful dock).

export module gb.presentation.ship_ops;

export import gb.presentation.core;

import gb.entities;
import gb.services;
import gb.mechanics;
import std;

export namespace GB::presentation {

/// Formats a `DetonateError` into its ASCII error string.
[[nodiscard]] std::string render_detonate_error(DetonateError err);

/// Formats a `JettisonError` into its ASCII error string.
[[nodiscard]] std::string render_jettison_error(const JettisonError& err);

/// Formats a `JettisonResult` into its ASCII presentation string.
[[nodiscard]] std::string render_jettison_result(const JettisonResult& res);

/// Formats a `MountCrystalError` into its ASCII error string.
[[nodiscard]] std::string render_mount_crystal_error(MountCrystalError err);

/// Formats a `MountCrystalResult` into its ASCII presentation string.
[[nodiscard]] std::string
render_mount_crystal_result(const MountCrystalResult& res);

/// Formats a `DismountCrystalError` into its ASCII error string.
[[nodiscard]] std::string
render_dismount_crystal_error(DismountCrystalError err);

/// Formats a `DismountCrystalResult` into its ASCII presentation string.
[[nodiscard]] std::string
render_dismount_crystal_result(const DismountCrystalResult& res);

/// Formats a `GrantShipResult` into its caller ASCII presentation string.
[[nodiscard]] std::string render_grant_ship_result(const GrantShipResult& res);

/// Formats a `GrantShipResult` into the recipient governor's notification.
[[nodiscard]] std::string
render_grant_ship_notification(const GrantShipResult& res);

/// Formats a `ScrapError` into its ASCII error string.
[[nodiscard]] std::string render_scrap_error(ScrapError err);

/// Formats a `ScrapShipResult` into its ASCII presentation string.
[[nodiscard]] std::string render_scrap_ship_result(const ScrapShipResult& res);

/// Formats a `PeacefulDockError` into its ASCII error string.
[[nodiscard]] std::string
render_peaceful_dock_error(const PeacefulDockError& err);

/// Formats a `PeacefulDockResult` into its ASCII presentation string.
[[nodiscard]] std::string
render_peaceful_dock_result(const PeacefulDockResult& res);

/// Open ADL dispatch for `JettisonResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const JettisonResult& vm);

/// Open ADL dispatch for `MountCrystalResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const MountCrystalResult& vm);

/// Open ADL dispatch for `DismountCrystalResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const DismountCrystalResult& vm);

/// Open ADL dispatch for `GrantShipResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const GrantShipResult& vm);

/// Open ADL dispatch for `ScrapShipResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ScrapShipResult& vm);

/// Open ADL dispatch for `PeacefulDockResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const PeacefulDockResult& vm);

}  // namespace GB::presentation
