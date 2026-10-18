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
  OnlyMissilesCanScatter,
  OnlyMissilesCanBeDesignated,
  InvalidDesignateCoords,
  UseOnForFactory,
  NoSwitchSetting,
  ShipBeingTransported,
  NotTerraformerOrPlow,
  EmptyCyclingMoveOrders,
  InvalidMoveDirection,
  CannotAssignTriggerRadius,
  NotATransporter,
  CannotTransportToSelf,
  CannotAimShip,
  NotEnoughManeuveringFuel,
  MirrorDocked,
  AimDestinationError,
  AimPlaceError,
  ThisShipHasNoSwitch,
  DamagedShipsCannotBeActivated,
  ShipAlreadyActivated,
  FactoryBeingTransported,
  InsufficientHabitatResourcesForFactory,
  InsufficientHabitatHangarForFactory,
  CannotActivateFactoryHere,
  InsufficientPlanetResourcesForFactory,
  CannotDeactivateFactory,
};

export struct OrderError {
  OrderErrorReason reason{OrderErrorReason::CannotBeAssignedOrders};
  std::string ship_display{};
  radiation_t radiation{0};
  std::optional<PlaceError> place_error{std::nullopt};
  char invalid_move_char{'\0'};
  double required_fuel{0.0};
  resource_t required_resources{0};
  shipnum_t habitat_ship{0};
  int hangar_needed{0};
};

export enum class OrderUpdateNotice {
  None,
  MineArmed,
  MineDisarmed,
  TransporterReady,
  TransporterStopped,
  MoveTruncatedLength,
  MoveTruncatedAfterModeChar,
  TransportTargetSet,
  Aimed,
  FactoryActivated,
};

export enum class TelescopeSurveyOutcome {
  None,
  NothingAtUniv,
  NothingOfUseAtShip,
  StarSurveyed,
  StarTooFar,
  PlanetSurveyed,
  PlanetTooFar,
};

export struct OrderUpdate {
  bool modified{true};
  OrderUpdateNotice notice{OrderUpdateNotice::None};
  std::size_t max_moves{0};
  char truncated_after_char{'\0'};
  shipnum_t target_ship{0};
  std::string aim_target{};
  TelescopeSurveyOutcome survey_outcome{TelescopeSurveyOutcome::None};
  double survey_distance{0.0};
  double tele_range{0.0};
  resource_t factory_activation_cost{0};
};

export ShipOrderStatus query_ship_order(EntityManager& em, const Ship& ship);
export std::expected<OrderUpdate, OrderError>
give_orders(EntityManager& em, const ScopeContext& scope_ctx,
            const command_t& argv, Ship& ship);

export enum class JettisonCommodity {
  Crystals,
  Crew,
  Military,
  Destruct,
  Fuel,
  Resources,
};

export enum class JettisonErrorReason {
  ShipLanded,
  ShipIrradiated,
  InvalidCommodity,
  NegativeAmount,
  ExceedsAvailable,
  NothingToJettison,
};

export struct JettisonError {
  JettisonErrorReason reason{JettisonErrorReason::InvalidCommodity};
  std::string ship_display{};
  int max_available{0};
};

export struct JettisonResult {
  JettisonCommodity commodity{JettisonCommodity::Resources};
  int amount{0};
  std::string ship_display{};
  shipnum_t ship_number{0};
  population_t remaining_complement{0};
  bool check_boobytrap{false};
  bool still_boobytrapped{false};
};

export std::expected<JettisonResult, JettisonError>
jettison_ship_cargo(Ship& ship, char commod, int requested_amount,
                    double race_mass);

export enum class MountCrystalError {
  NoCrystalMount,
  AlreadyMounted,
  NoCrystalsOnBoard,
};

export struct MountCrystalResult {};

export std::expected<MountCrystalResult, MountCrystalError>
mount_ship_crystal(Ship& ship);

export enum class DismountCrystalError {
  NoCrystalMount,
  NotMounted,
  MaxCrystalsOnBoard,
};

export struct DismountCrystalResult {
  bool hyperdrive_discharged{false};
  bool laser_deactivated{false};
};

export std::expected<DismountCrystalResult, DismountCrystalError>
dismount_ship_crystal(Ship& ship);

export struct GrantShipResult {
  std::string ship_display{};
  std::string orbits_display{};
  std::string donor_governor_name{};
  std::string recipient_governor_name{};
};

export GrantShipResult grant_ship_governor(EntityManager& em, Ship& ship,
                                           governor_t new_gov,
                                           std::string_view donor_gov_name,
                                           std::string_view recipient_gov_name);

export struct CargoTransferLimits {
  std::int64_t lolim{0};
  std::int64_t uplim{0};
};

export CargoTransferLimits
compute_planet_transfer_limits(EntityManager& em, const Ship& s,
                               player_t player, ShipCargoType cargo,
                               starnum_t snum, planetnum_t pnum);

export enum class PlanetSectorEvent {
  None,
  Colonized,
  Occupied,
  Evacuated,
};

export inline constexpr std::array all_planet_sector_events = {
    PlanetSectorEvent::None,
    PlanetSectorEvent::Colonized,
    PlanetSectorEvent::Occupied,
    PlanetSectorEvent::Evacuated,
};

export struct AlienSectorAssaultResult {
  bool requires_unload{false};
  PopulationType attacker_type{PopulationType::CIV};
  player_t attacker_player{0};
  governor_t attacker_governor{0};
  std::string attacker_race_name{};
  player_t defender_player{0};
  governor_t defender_governor{0};
  std::string defender_race_name{};
  std::string star_name{};
  std::string planet_name{};
  std::string ship_display{};
  Coordinates sector_coords{};
  char sector_condition_symbol{' '};
  population_t unloaded_people{0};
  population_t ship_civ_after_unload{0};
  population_t ship_mil_after_unload{0};
  population_t initial_defender_civ{0};
  population_t initial_defender_mil{0};
  double attack_strength{0.0};
  double defense_strength{0.0};
  bool defender_wiped_out{false};
  bool attacker_won{false};
  bool attacker_absorbed_bodies{false};
  bool defender_absorbed_bodies{false};
  int absorbed_bodies{0};
  population_t surviving_attackers{0};
  population_t attacker_casualties{0};
  population_t defender_civ_casualties{0};
  population_t defender_mil_casualties{0};
};

export struct PlanetCargoTransferResult {
  ShipCargoType commodity{ShipCargoType::Resource};
  std::int64_t amount{0};
  std::string ship_display{};
  Coordinates sector_coords{};
  PlanetSectorEvent sector_event{PlanetSectorEvent::None};
  population_t resulting_popn{0};
  population_t resulting_troops{0};
  bool check_boobytrap{false};
  bool is_boobytrapped{false};
  std::optional<AlienSectorAssaultResult> assault{std::nullopt};
};

export PlanetCargoTransferResult
execute_planet_cargo_transfer(EntityManager& em, Ship& s, player_t player,
                              governor_t governor, starnum_t snum,
                              planetnum_t pnum, ShipCargoType cargo,
                              std::int64_t amt, double race_mass);

export enum class ShipDockingErrorReason {
  Inactive,
  NotLandedOrDocked,
  WrongPlanetScope,
  NotDocked,
  TargetNotDocked,
  TargetOverloaded,
};

export inline constexpr std::array all_ship_docking_error_reasons = {
    ShipDockingErrorReason::Inactive,
    ShipDockingErrorReason::NotLandedOrDocked,
    ShipDockingErrorReason::WrongPlanetScope,
    ShipDockingErrorReason::NotDocked,
    ShipDockingErrorReason::TargetNotDocked,
    ShipDockingErrorReason::TargetOverloaded,
};

export struct ShipDockingError {
  ShipDockingErrorReason reason{ShipDockingErrorReason::Inactive};
  std::string ship_display{};
  std::string target_ship_display{};
  Coordinates land_coords{};
};

export struct ShipDockingStatus {
  bool is_docked_to_ship{false};
  bool is_different_owner{false};
  std::string ship_display{};
  std::string target_ship_display{};
  Coordinates land_coords{};
  player_t target_owner{0};
};

export std::expected<ShipDockingStatus, ShipDockingError>
validate_ship_docking(EntityManager& em, Ship& s, player_t player,
                      ScopeLevel scope_level, starnum_t snum, planetnum_t pnum);

export CargoTransferLimits compute_ship_transfer_limits(const Ship& s,
                                                        const Ship& s2,
                                                        ShipCargoType cargo,
                                                        bool diff);

export struct ShipCargoTransferResult {
  ShipCargoType commodity{ShipCargoType::Resource};
  std::int64_t amount{0};
  std::string ship_display{};
  std::string target_ship_display{};
  player_t target_owner{0};
  governor_t target_governor{0};
  bool notify_target_owner{false};
  bool is_metamorph{false};
  population_t resulting_popn{0};
  population_t resulting_troops{0};
  bool check_boobytrap{false};
  bool is_boobytrapped{false};
};

export ShipCargoTransferResult
execute_ship_to_ship_cargo_transfer(EntityManager& em, Ship& s,
                                    ShipCargoType cargo, std::int64_t amt,
                                    double race_mass, bool is_metamorph);

export enum class TransporterError {
  OriginNotLanded,
  WrongPlanetScope,
  OriginDamaged,
  HopperBlocked,
  TargetNotReceiving,
  TargetNotLanded,
  TargetDamaged,
};

export inline constexpr std::array all_transporter_errors = {
    TransporterError::OriginNotLanded,    TransporterError::WrongPlanetScope,
    TransporterError::OriginDamaged,      TransporterError::HopperBlocked,
    TransporterError::TargetNotReceiving, TransporterError::TargetNotLanded,
    TransporterError::TargetDamaged,
};

export struct TransporterCommodityTransfer {
  ShipCargoType commodity{ShipCargoType::Resource};
  std::int64_t amount{0};
};

export struct TransporterTransferResult {
  std::string sender_ship_display{};
  std::string receiver_ship_display{};
  player_t receiver_owner{0};
  governor_t receiver_governor{0};
  bool notify_receiver{false};
  bool is_metamorph{false};
  std::vector<TransporterCommodityTransfer> transfers{};
};

export std::expected<TransporterTransferResult, TransporterError>
execute_transporter_transfer(EntityManager& em, TransporterShip& s,
                             starnum_t snum, planetnum_t pnum, double race_mass,
                             bool is_metamorph);

export enum class LaunchErrorReason {
  NotDesignedToLaunch,
  NotLandedOrDocked,
  Overloaded,
  OnlineFactoryCannotLaunch,
  MothershipBerthed,
  InsufficientUniverseAp,
  InsufficientStarAp,
  InsufficientFuel,
};

export inline constexpr std::array all_launch_error_reasons = {
    LaunchErrorReason::NotDesignedToLaunch,
    LaunchErrorReason::NotLandedOrDocked,
    LaunchErrorReason::Overloaded,
    LaunchErrorReason::OnlineFactoryCannotLaunch,
    LaunchErrorReason::MothershipBerthed,
    LaunchErrorReason::InsufficientUniverseAp,
    LaunchErrorReason::InsufficientStarAp,
    LaunchErrorReason::InsufficientFuel,
};

export struct LaunchError {
  LaunchErrorReason reason{LaunchErrorReason::NotDesignedToLaunch};
  std::string ship_display{};
  std::string target_ship_display{};
  std::string star_name{};
  std::string planet_name{};
  std::optional<double> planet_gravity{std::nullopt};
  double required_fuel{0.0};
};

export enum class LaunchMode {
  DeployedFromLandedCarrier,
  LaunchedFromOrbitingCarrier,
  UndockedFromShip,
  LaunchedFromPlanet,
};

export struct LaunchObserverTarget {
  player_t player{0};
  governor_t governor{0};
};

export struct LaunchShipResult {
  LaunchMode mode{LaunchMode::LaunchedFromPlanet};
  std::string ship_display{};
  std::string target_ship_display{};
  ScopeLevel orbit_scope{ScopeLevel::LEVEL_PLAN};
  std::string star_name{};
  std::string planet_name{};
  double planet_gravity{0.0};
  double fuel_used{0.0};
  ShipType ship_type{ShipType::STYPE_SHUTTLE};
  bool is_canister{false};
  std::vector<LaunchObserverTarget> observers{};
};

export std::expected<LaunchShipResult, LaunchError>
launch_single_ship(EntityManager& em, Ship& s, player_t player, bool god);
