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

/// Flags selecting which ship report sections to include in an `RstViewModel`.
struct RstReportFlags {
  bool status{false};
  bool ship{false};
  bool stock{false};
  bool report{false};
  bool weapons{false};
  bool factories{false};
};

/// Inventory and cargo row for `stock` / `ship` reports.
struct RstStockRow {
  shipnum_t number{0};
  char type_letter{' '};
  bool active{false};
  std::string name{};
  crystal_t crystals{0};
  hangar_t hanger{0};
  hangar_t max_hanger{0};
  resource_t resource{0};
  resource_t max_resource{0};
  resource_t destruct{0};
  resource_t max_destruct{0};
  double fuel{0.0};
  fuel_t max_fuel{0};
  population_t popn{0};
  population_t troops{0};
  population_t max_crew{0};
};

/// Capability and combat statistics row for `stats` / `ship` reports.
struct RstStatusRow {
  shipnum_t number{0};
  char type_letter{' '};
  bool active{false};
  std::string name{};
  bool has_laser{false};
  bool has_cew{false};
  bool has_hyperdrive{false};
  std::string battery_summary{};
  armor_t effective_armor{0};
  double tech{0.0};
  speed_t max_speed{0};
  long effective_cost{0};
  double mass{0.0};
  ship_size_t size{0};
  std::optional<int> pod_temperature{std::nullopt};
};

/// Weapons and ordnance row for `weapons` / `ship` reports.
struct RstWeaponsRow {
  shipnum_t number{0};
  char type_letter{' '};
  bool active{false};
  std::string name{};
  bool has_laser{false};
  weapon_power_t cew{0};
  weapon_range_t cew_range{0};
  int safe_cew{0};
  std::string battery_summary{};
  damage_t damage{0};
  std::string ship_class{};
};

/// Factory blueprint row for `factories` / `ship` reports.
struct RstFactoryRow {
  shipnum_t number{0};
  bool unconfigured{false};
  char build_type_letter{' '};
  resource_t build_cost{0};
  double complexity{0.0};
  double base_mass{0.0};
  ship_size_t size{0};
  armor_t armor{0};
  population_t max_crew{0};
  fuel_t max_fuel{0};
  resource_t max_resource{0};
  hangar_t max_hanger{0};
  resource_t max_destruct{0};
  bool has_hyperdrive{false};
  bool has_mount{false};
  speed_t max_speed{0};
  std::string primary_guns{};
  std::string secondary_guns{};
  bool has_laser{false};
  weapon_power_t cew{0};
  weapon_range_t cew_range{0};
  damage_t damage{0};
  bool on{false};
};

/// Summary status and navigation row for `report` / `ship` reports.
struct RstGeneralRow {
  char type_letter{' '};
  shipnum_t number{0};
  bool active{false};
  std::string name{};
  radiation_t radiation{0};
  governor_t governor{0};
  damage_t damage{0};
  population_t popn{0};
  population_t troops{0};
  resource_t destruct{0};
  double fuel{0.0};
  bool has_hyperdrive{false};
  bool mounted{false};
  speed_t speed{0};
  std::string orbits{};
  bool docked{false};
  ScopeLevel whatdest{ScopeLevel::LEVEL_UNIV};
  std::optional<shipnum_t> dest_shipno{std::nullopt};
  Coordinates land_coords{0, 0};
  bool navigating{false};
  bearing_t nav_bearing{0};
  std::uint32_t nav_turns{0};
  std::string dest_path{};
};

/// Per-ship bundle of report rows collected for an `RstViewModel`.
struct RstShipEntry {
  std::optional<RstStockRow> stock{std::nullopt};
  std::optional<RstStatusRow> status{std::nullopt};
  std::optional<RstWeaponsRow> weapons{std::nullopt};
  std::optional<RstFactoryRow> factory{std::nullopt};
  std::optional<RstGeneralRow> general{std::nullopt};
};

/// View model for `report`, `ship`, `stats`, `stock`, `weapons`, and
/// `factories` commands.
struct RstViewModel {
  RstReportFlags flags{};
  std::vector<RstShipEntry> ships{};
};

/// Formats an `RstViewModel` into its ASCII tabular presentation string.
[[nodiscard]] std::string render_rst(const RstViewModel& vm);

/// Formats an `RstViewModel` into its JSON envelope string.
[[nodiscard]] std::string render_json_rst(const RstViewModel& vm);

/// Open ADL dispatch for `RstViewModel`.
[[nodiscard]] std::string render_for_mode(UiMode mode, const RstViewModel& vm);

}  // namespace GB::presentation
