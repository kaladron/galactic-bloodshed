// SPDX-License-Identifier: Apache-2.0

/// \file combat.cppm
/// \brief Tier-6a Presentation submodule for ship, planetary, boarding, and
/// ground combat results and errors.

export module gb.presentation.combat;

export import gb.presentation.core;

import gb.entities;
import gb.services;
import gb.mechanics;
import std;

export namespace GB::presentation {

/// Formats a `ReactorOverloadEvent` into its ASCII combat report string.
[[nodiscard]] std::string
render_reactor_overload_event(const ReactorOverloadEvent& event);

/// Formats a `MechAttackPeopleResult` into its short headline string (used for
/// star notifications and combat news).
[[nodiscard]] std::string
render_mech_attack_people_short(const MechAttackPeopleResult& res);

/// Formats a `MechAttackPeopleResult` into its full multi-line battle report.
[[nodiscard]] std::string
render_mech_attack_people_long(const MechAttackPeopleResult& res);

/// Formats a `PeopleAttackMechResult` into its short headline string (used for
/// star notifications and combat news).
[[nodiscard]] std::string
render_people_attack_mech_short(const PeopleAttackMechResult& res);

/// Formats a `PeopleAttackMechResult` into its full multi-line battle report.
[[nodiscard]] std::string
render_people_attack_mech_long(const PeopleAttackMechResult& res);

/// Formats the short one-line summary for `ShipShotResult`
/// (`shoot_ship_to_ship` and `shoot_planet_to_ship`).
[[nodiscard]] std::string render_ship_shot_short(const ShipShotResult& res);

/// Formats the full multi-line combat report for `ShipShotResult`.
[[nodiscard]] std::string render_ship_shot_long(const ShipShotResult& res);

/// Formats the short one-line summary for `BombardResult`
/// (`shoot_ship_to_planet`).
[[nodiscard]] std::string render_bombard_short(const BombardResult& res);

/// Formats the full multi-line report for `BombardResult`
/// (`shoot_ship_to_planet`).
[[nodiscard]] std::string render_bombard_long(const BombardResult& res);

/// Formats the combat news/star notification line for `MineDetonationReport`.
[[nodiscard]] std::string
render_mine_detonation_notice(const MineDetonationReport& report);

/// Formats the planetary strike telegram for `MineDetonationReport`.
[[nodiscard]] std::string
render_mine_planet_strike_telegram(const MineDetonationReport& report);

/// Formats the full interactive command report for `MineDetonationReport`.
[[nodiscard]] std::string
render_mine_detonation_report(const MineDetonationReport& report);

/// Formats an `AssaultError` into its ASCII error string.
[[nodiscard]] std::string render_assault_error(const AssaultError& err);

/// Formats a `BoardingOutcomeReport` into the defender's warning telegram.
[[nodiscard]] std::string
render_boarding_defender_telegram(const BoardingOutcomeReport& b);

/// Formats a `BoardingOutcomeReport` into a combat news/star notification line.
[[nodiscard]] std::string render_boarding_news(const BoardingOutcomeReport& b);

/// Formats a `ShipCombatExchange` into its ASCII presentation string.
[[nodiscard]] std::string
render_ship_combat_exchange(const ShipCombatExchange& df);

/// Formats a `BoardingOutcomeReport` into its ASCII presentation string.
[[nodiscard]] std::string
render_boarding_outcome(const BoardingOutcomeReport& b);

/// Formats an `AssaultResult` into its ASCII presentation string.
[[nodiscard]] std::string render_assault_result(const AssaultResult& res);

/// Formats the primary error message for a `FireErrorReason`.
[[nodiscard]] std::string render_fire_error_reason(const FireError& err);

/// Formats a `FireError` into its ASCII diagnostic string.
[[nodiscard]] std::string render_fire_error(const FireError& err);

/// Formats a `FireShipResult` into its ASCII presentation string.
[[nodiscard]] std::string render_fire_ship_result(const FireShipResult& res);

/// Open ADL dispatch for `ReactorOverloadEvent`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ReactorOverloadEvent& vm);

/// Open ADL dispatch for `MechAttackPeopleResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const MechAttackPeopleResult& vm);

/// Open ADL dispatch for `PeopleAttackMechResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const PeopleAttackMechResult& vm);

/// Open ADL dispatch for `ShipShotResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ShipShotResult& vm);

/// Open ADL dispatch for `BombardResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode, const BombardResult& vm);

/// Open ADL dispatch for `MineDetonationReport`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const MineDetonationReport& vm);

/// Open ADL dispatch for `AssaultResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode, const AssaultResult& vm);

/// Open ADL dispatch for `FireShipResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const FireShipResult& vm);

/// Header row for a ship firer in a tactical report.
struct TacticalShipHeader {
  shipnum_t number{0};
  char type_letter{' '};
  bool active{false};
  std::string name{};
  double tech{0.0};
  std::string battery_summary{};
  armor_t armor{0};
  ship_size_t size{0};
  resource_t destruct{0};
  double fuel{0.0};
  damage_t damage{0};
  speed_t fspeed{0};
  bool evade{false};
  std::string orbits{};
  std::optional<Coordinates> landed_coords{std::nullopt};
  radiation_t radiation{0};
};

/// Header row for a planetary firer in a tactical report.
struct TacticalPlanetHeader {
  std::string planet_name{};
  double tech{0.0};
  gun_count_t guns{0};
  resource_t destruct{0};
  resource_t fuel{0};
};

/// Target row for a ship target in a tactical report.
struct TacticalShipTargetRow {
  shipnum_t number{0};
  char relation_indicator{' '};
  player_t owner{0};
  governor_t governor{0};
  char type_letter{' '};
  std::string name{};
  double distance{0.0};
  weapon_range_t range_50_pct{0};
  ship_size_t size{0};
  speed_t speed{0};
  bool evade{false};
  hit_odds_t hit_prob{0};
  damage_t damage{0};
  std::optional<Coordinates> landed_coords{std::nullopt};
  bool active{false};
};

/// Target row for a planetary target in a tactical report.
struct TacticalPlanetTargetRow {
  std::string planet_name{};
  double distance{0.0};
};

/// A single target row (either ship or planet) within a tactical report entry.
struct TacticalTargetEntry {
  std::optional<TacticalShipTargetRow> ship{std::nullopt};
  std::optional<TacticalPlanetTargetRow> planet{std::nullopt};
};

/// A complete tactical report section for a single firer (ship or planet) and
/// its targets in range.
struct TacticalReportEntry {
  std::optional<TacticalShipHeader> ship_header{std::nullopt};
  std::optional<TacticalPlanetHeader> planet_header{std::nullopt};
  std::vector<TacticalTargetEntry> targets{};
};

/// View model for the `tactical` command.
struct TacticalViewModel {
  std::vector<TacticalReportEntry> entries{};
};

/// Formats a `TacticalViewModel` into its ASCII tabular presentation string.
[[nodiscard]] std::string render_tactical(const TacticalViewModel& vm);

/// Formats a `TacticalViewModel` into its JSON envelope string.
[[nodiscard]] std::string render_json_tactical(const TacticalViewModel& vm);

/// Open ADL dispatch for `TacticalViewModel`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const TacticalViewModel& vm);

/// Formats a `MilitiaError` into its user-facing error message.
[[nodiscard]] std::string format_militia_error(const MilitiaError& err);

/// Formats an `ArmSectorResult` into its user-facing ASCII report.
[[nodiscard]] std::string render_arm_sector_result(const ArmSectorResult& vm);

/// Formats an `ArmSectorResult` into its JSON envelope string.
[[nodiscard]] std::string
render_json_arm_sector_result(const ArmSectorResult& vm);

/// Open ADL dispatch for `ArmSectorResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const ArmSectorResult& vm);

/// Formats a `DisarmSectorResult` into its user-facing ASCII report.
[[nodiscard]] std::string
render_disarm_sector_result(const DisarmSectorResult& vm);

/// Formats a `DisarmSectorResult` into its JSON envelope string.
[[nodiscard]] std::string
render_json_disarm_sector_result(const DisarmSectorResult& vm);

/// Open ADL dispatch for `DisarmSectorResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const DisarmSectorResult& vm);

/// Formats a `SectorGroundAssaultResult` into the attacker's ASCII battle
/// report.
[[nodiscard]] std::string
render_sector_ground_assault(const SectorGroundAssaultResult& res);

/// Formats the optional Metamorph body-absorption warning telegram for the
/// defender.
[[nodiscard]] std::optional<std::string>
render_sector_ground_assault_absorb_warning(
    const SectorGroundAssaultResult& res);

/// Formats the defender's battle summary telegram for a sector ground assault.
[[nodiscard]] std::string
render_sector_ground_assault_telegram(const SectorGroundAssaultResult& res);

/// Formats a `MovePopnError` into its user-facing error message.
[[nodiscard]] std::string format_move_popn_error(const MovePopnError& err);

/// Formats a `MovePopnResult` into its user-facing ASCII report.
[[nodiscard]] std::string render_move_popn_result(const MovePopnResult& res);

/// Formats a `MovePopnResult` into its JSON envelope string.
[[nodiscard]] std::string
render_json_move_popn_result(const MovePopnResult& res);

/// Open ADL dispatch for `MovePopnResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const MovePopnResult& vm);

/// Formats a `BombardError` into its user-facing error message.
[[nodiscard]] std::string format_bombard_error(const BombardError& err);

/// Formats a `BombardShipResult` into its user-facing ASCII report.
[[nodiscard]] std::string
render_bombard_ship_result(const BombardShipResult& res);

/// Formats a `BombardShipResult` into its JSON envelope string.
[[nodiscard]] std::string
render_json_bombard_ship_result(const BombardShipResult& res);

/// Open ADL dispatch for `BombardShipResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const BombardShipResult& vm);

/// Formats a `DefendError` into its user-facing error message.
[[nodiscard]] std::string format_defend_error(const DefendError& err);

/// Formats a `DefendPlanetResult` into its user-facing ASCII report.
[[nodiscard]] std::string
render_defend_planet_result(const DefendPlanetResult& res);

/// Formats a `DefendPlanetResult` into its JSON envelope string.
[[nodiscard]] std::string
render_json_defend_planet_result(const DefendPlanetResult& res);

/// Open ADL dispatch for `DefendPlanetResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const DefendPlanetResult& vm);

/// Formats a `WalkError` into its user-facing error message.
[[nodiscard]] std::string format_walk_error(const WalkError& err);

/// Formats the planetary observer warning telegram for an AFV sector walk.
[[nodiscard]] std::string render_walk_observer_warning(const WalkResult& res);

/// Formats a `WalkResult` into its user-facing ASCII report.
[[nodiscard]] std::string render_walk_result(const WalkResult& res);

/// Formats a `WalkResult` into its JSON envelope string.
[[nodiscard]] std::string render_json_walk_result(const WalkResult& res);

/// Open ADL dispatch for `WalkResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode, const WalkResult& vm);

/// Formats a `CaptureError` into its user-facing error message.
[[nodiscard]] std::string format_capture_error(const CaptureError& err);

/// Formats a `LandedBoardingResult` into the defending owner's warning
/// telegram.
[[nodiscard]] std::string
render_capture_defender_telegram(const LandedBoardingResult& res);

/// Formats a `LandedBoardingResult` into a combat news/star notification line.
[[nodiscard]] std::string render_capture_news(const LandedBoardingResult& res);

/// Formats a `LandedBoardingResult` into its user-facing ASCII report.
[[nodiscard]] std::string
render_capture_result(const LandedBoardingResult& res);

/// Formats a `LandedBoardingResult` into its JSON envelope string.
[[nodiscard]] std::string
render_json_capture_result(const LandedBoardingResult& res);

/// Open ADL dispatch for `LandedBoardingResult`.
[[nodiscard]] std::string render_for_mode(UiMode mode,
                                          const LandedBoardingResult& vm);

/// Turn bulletin payload when a warming spore pod explodes in star orbit.
struct SporePodExplosionNotice {
  std::string pod_display{};
  std::string orbit_path{};
  std::optional<std::string> colonized_planet_name{std::nullopt};
};

/// Formats a `SporePodExplosionNotice` into its owner telegram.
[[nodiscard]] std::string
render_spore_pod_explosion(const SporePodExplosionNotice& vm);

/// Formats the decay telegram for a spore pod in planet orbit.
[[nodiscard]] std::string render_spore_pod_decay(std::string_view pod_display,
                                                 std::string_view orbit_path);

/// Formats the planetary dissipation telegram for a dust canister or greenhouse
/// gas device.
[[nodiscard]] std::string
render_atmosphere_device_dissipated(std::string_view orbit_path,
                                    bool greenhouse);

/// Turn bulletin payload when a space mirror focuses thermal energy on a ship.
struct MirrorShipStrikeNotice {
  std::string mirror_display{};
  std::string target_display{};
  damage_t damage_applied{0};
  bool destroyed{false};
};

/// Formats a `MirrorShipStrikeNotice` into its combat telegram.
[[nodiscard]] std::string
render_mirror_ship_strike(const MirrorShipStrikeNotice& vm);

/// Formats a planetary missile drop into the bystander colony telegram.
[[nodiscard]] std::string
render_missile_planet_drop_telegram(const BombardResult& res,
                                    std::string_view orbit_path);

/// Formats a planetary missile drop into its combat news headline.
[[nodiscard]] std::string
render_missile_planet_drop_news(const BombardResult& res,
                                std::string_view orbit_path);

}  // namespace GB::presentation
