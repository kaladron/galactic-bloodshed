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

/// Formats a `FactoryMakeError` into its user-facing error diagnostic string.
[[nodiscard]] std::string format_factory_make_error(FactoryMakeError err);

/// Formats a `FactoryModifyError` into its user-facing error diagnostic string.
[[nodiscard]] std::string format_factory_modify_error(FactoryModifyError err);

/// Formats a `FactorySpecsViewModel` into its ASCII specification table.
[[nodiscard]] std::string render_factory_specs(const FactorySpecsViewModel& vm);

/// Formats a `FactorySpecsViewModel` into its JSON envelope.
[[nodiscard]] std::string
render_json_factory_specs(const FactorySpecsViewModel& vm);

/// Formats a `FactoryMakeResult` into its ASCII presentation string.
[[nodiscard]] std::string
render_factory_make_result(const FactoryMakeResult& vm);

/// Formats a `FactoryMakeResult` into its JSON envelope.
[[nodiscard]] std::string
render_json_factory_make_result(const FactoryMakeResult& vm);

/// Formats a `FactoryModifyResult` into its ASCII presentation string.
[[nodiscard]] std::string
render_factory_modify_result(const FactoryModifyResult& vm);

/// Formats a `FactoryModifyResult` into its JSON envelope.
[[nodiscard]] std::string
render_json_factory_modify_result(const FactoryModifyResult& vm);

/// Open ADL dispatch for `FactorySpecsViewModel`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const FactorySpecsViewModel& vm);

/// Open ADL dispatch for `FactoryMakeResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const FactoryMakeResult& vm);

/// Open ADL dispatch for `FactoryModifyResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const FactoryModifyResult& vm);

/// Row in the default ship parameters table (`build ?` / `build ? <type>`).
struct ShipSpecRow {
  std::string letter{};
  std::string name{};
  resource_t max_resource{0};
  hangar_t max_hangar{0};
  armor_t base_armor{0};
  resource_t max_destruct{0};
  gun_count_t max_guns{0};
  std::string primary_caliber{};
  std::string secondary_caliber{};
  fuel_t max_fuel{0.0};
  population_t max_crew{0};
  speed_t base_speed{0};
  double base_tech{0.0};
  resource_t cost{0};
};

/// View model for the full ship specifications table (`build ?`).
struct ShipSpecsTableViewModel {
  std::vector<ShipSpecRow> rows{};
};

/// View model for a specific ship type specification (`build ? <type>`).
struct ShipSpecDetailViewModel {
  std::optional<std::string> description{std::nullopt};
  bool can_build_on_planet{false};
  std::vector<std::string> builder_letters{};
  ShipSpecRow spec{};
};

/// Builds a `ShipSpecRow` for a single ship template and race.
[[nodiscard]] ShipSpecRow build_ship_spec_row(ShipType type, const Race& race);

/// Builds a `ShipSpecsTableViewModel` for all ship types available to `race`.
[[nodiscard]] ShipSpecsTableViewModel
build_ship_specs_table_vm(const Race& race);

/// Builds a `ShipSpecDetailViewModel` for a specific ship type and race.
[[nodiscard]] ShipSpecDetailViewModel build_ship_spec_detail_vm(
    ShipType type, const Race& race,
    std::optional<std::string_view> description = std::nullopt);

/// Formats a `ShipSpecsTableViewModel` into its ASCII specification table.
[[nodiscard]] std::string
render_ship_specs_table(const ShipSpecsTableViewModel& vm);

/// Formats a `ShipSpecsTableViewModel` into its JSON envelope.
[[nodiscard]] std::string
render_json_ship_specs_table(const ShipSpecsTableViewModel& vm);

/// Formats a `ShipSpecDetailViewModel` into its ASCII presentation string.
[[nodiscard]] std::string
render_ship_spec_detail(const ShipSpecDetailViewModel& vm);

/// Formats a `ShipSpecDetailViewModel` into its JSON envelope.
[[nodiscard]] std::string
render_json_ship_spec_detail(const ShipSpecDetailViewModel& vm);

/// Open ADL dispatch for `ShipSpecsTableViewModel`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ShipSpecsTableViewModel& vm);

/// Open ADL dispatch for `ShipSpecDetailViewModel`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ShipSpecDetailViewModel& vm);

}  // namespace GB::presentation
