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
  resource_t crystals{0};
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

using MirrorShip = SpaceMirrorShip;

/// Cache of visual sight checks across star systems and planet surfaces for
/// multi-ship projection.
class SightContext {
public:
  [[nodiscard]] bool has_star_sight(EntityManager& em, player_t player,
                                    starnum_t snum);
  [[nodiscard]] bool has_planet_sight(EntityManager& em, player_t player,
                                      starnum_t snum, planetnum_t pnum);

private:
  std::flat_map<std::pair<player_t, starnum_t>, bool,
                std::less<std::pair<player_t, starnum_t>>,
                std::vector<std::pair<player_t, starnum_t>>, std::deque<bool>>
      star_cache_{};
  std::flat_map<std::tuple<player_t, starnum_t, planetnum_t>, bool,
                std::less<std::tuple<player_t, starnum_t, planetnum_t>>,
                std::vector<std::tuple<player_t, starnum_t, planetnum_t>>,
                std::deque<bool>>
      planet_cache_{};
};

/// Spatial coordinates and physical mooring status for a ship.
struct ShipLocationView {
  ScopeLevel level{ScopeLevel::LEVEL_UNIV};
  std::optional<starnum_t> snum{std::nullopt};
  std::optional<planetnum_t> pnum{std::nullopt};
  bool docked{false};
  bool landed{false};
  std::optional<shipnum_t> dest_shipno{std::nullopt};
  Coordinates land_coords{0, 0};
  UniverseCoordinates coordinates{0.0, 0.0};
  std::string orbits_path{};
  std::optional<int> mirror_orientation{std::nullopt};
};

/// Commodity and personnel inventory onboard a ship.
struct ShipCargoView {
  resource_t resource{0};
  resource_t max_resource{0};
  resource_t destruct{0};
  resource_t max_destruct{0};
  double fuel{0.0};
  fuel_t max_fuel{0};
  resource_t crystals{0};
  population_t crew{0};
  population_t max_crew{0};
  population_t troops{0};
  hangar_t hanger{0};
  hangar_t max_hanger{0};
};

/// Navigation route, hyperdrive, and escort directives for a ship.
struct ShipNavigationView {
  speed_t max_speed{0};
  bool has_mount{false};
  bool crystal_mounted{false};
  HyperDriveData hyper_drive{};
  ScopeLevel dest_level{ScopeLevel::LEVEL_UNIV};
  std::optional<starnum_t> dest_star{std::nullopt};
  std::optional<planetnum_t> dest_planet{std::nullopt};
  std::optional<shipnum_t> dest_ship{std::nullopt};
  std::string dest_path{};
  NavigateData navigate{};
  ProtectData protect{};
  int merchant_route{0};
};

/// Armament, combat laser, CEW projector, and retaliation settings.
struct ShipWeaponsView {
  armor_t armor{0};
  armor_t effective_armor{0};
  GunBattery primary{};
  GunBattery secondary{};
  std::string battery_summary{};
  bool has_laser{false};
  bool laser_on{false};
  bool laser_focused{false};
  int laser_strength{0};
  weapon_power_t cew_power{0};
  weapon_range_t cew_range{0};
  int safe_cew{0};
  weapon_power_t retaliate_salvo{0};
};

/// Factory ship construction blueprint specifications.
struct ShipFactoryBlueprintView {
  bool configured{false};
  ShipType build_type{ShipType::OTYPE_FACTORY};
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
  GunBattery primary{};
  GunBattery secondary{};
  bool has_laser{false};
  weapon_power_t cew_power{0};
  weapon_range_t cew_range{0};
  bool on{false};
};

/// Internal telemetry, cargo, navigation, and weapon systems visible to ship
/// owner or authorized viewer.
struct ShipInternalView {
  double tech{0.0};
  double base_mass{0.0};
  double mass{0.0};
  long effective_cost{0};
  radiation_t radiation{0};
  bool on{false};
  ShipCargoView cargo{};
  ShipNavigationView navigation{};
  ShipWeaponsView weapons{};
  std::optional<ShipFactoryBlueprintView> factory{std::nullopt};
  SpecialData special{};
};

/// Canonical view model for a starship, pod, or orbital installation.
struct ShipView {
  shipnum_t id{0};
  bool alive{true};
  player_t owner{0};
  governor_t governor{0};
  std::string name{};
  std::string shipclass{};
  ShipType type{ShipType::STYPE_POD};
  char type_letter{' '};
  ship_size_t size{0};
  speed_t speed{0};
  damage_t damage{0};
  bool active{false};
  std::optional<ShipLocationView> location{std::nullopt};
  std::optional<ShipInternalView> internal{std::nullopt};
};

/// Projects canonical ShipView under viewer scope and fog-of-war rules.
/// Returns std::nullopt if ship contact is hidden by fog-of-war.
[[nodiscard]] std::optional<ShipView>
project_ship_view(EntityManager& em, const ScopeContext& viewer,
                  const Ship& ship, SightContext* sight = nullptr);

/// Projects a tombstone ShipView representing a destroyed/non-existent ship.
[[nodiscard]] ShipView project_deleted_ship_view(shipnum_t id);

/// Formats a canonical ShipView into its JSON envelope string.
[[nodiscard]] std::string render_json_ship_view(const ShipView& vm);

}  // namespace GB::presentation
