// SPDX-License-Identifier: Apache-2.0

/// \file combat.cppm
/// \brief Space combat, orbital bombardment, crystal overload, and ground/AFV
/// combat mechanics.

module;

export module gb.mechanics:combat;

import :navigation;
import gb.entities;
import gb.services;
import std;

export bool has_planet_defense(EntityManager&, starnum_t, planetnum_t,
                               player_t);

export enum class ReactorOverloadOutcome {
  CrystalDamaged,
  ShipExploded,
};

export struct ReactorOverloadEvent {
  ReactorOverloadOutcome outcome{ReactorOverloadOutcome::CrystalDamaged};
  player_t owner{0};
  governor_t governor{0};
  ScopeLevel scope{ScopeLevel::LEVEL_UNIV};
  starnum_t star_id{0};
  std::string location_display{};
  std::string ship_display{};
};

export std::pair<weapon_power_t, std::optional<ReactorOverloadEvent>>
check_overload(EntityManager& entity_manager, Ship& ship, int cew,
               weapon_power_t strength);

/// \brief Collateral casualties and system damage inflicted on a target ship.
export struct CollateralDamage {
  population_t civilian_casualties{0};
  population_t military_casualties{0};
  gun_count_t primary_guns_lost{0};
  gun_count_t secondary_guns_lost{0};
};

export CollateralDamage do_collateral(Ship& ship, damage_t damage,
                                      double race_mass = 1.0);

export struct CriticalHitSystemsDamage {
  bool cew_destroyed{false};
  bool laser_destroyed{false};
  bool cloak_destroyed{false};
  bool hyper_drive_destroyed{false};
  std::optional<speed_t> reduced_max_speed{std::nullopt};
  std::optional<armor_t> reduced_armor{std::nullopt};
};

export struct CriticalHitResult {
  hit_count_t count{0};
  damage_t damage{0};
  CriticalHitSystemsDamage systems{};
};

export enum class ShipShotAttackerKind {
  Ship,
  Planet,
};

export enum class ShipShotWeaponKind {
  Radiation,
  Cew,
  FocusedLaser,
  Laser,
  LightGuns,
  MediumGuns,
  HeavyGuns,
};

export struct ShipShotResult {
  ShipShotAttackerKind attacker_kind{ShipShotAttackerKind::Ship};
  player_t attacker_player{0};
  std::string attacker_display{};
  std::string target_location_display{};
  std::string target_display{};
  bool target_alive{true};
  ShipShotWeaponKind weapon{ShipShotWeaponKind::LightGuns};
  weapon_power_t strength{0};
  double range{0.0};
  hit_count_t hits{0};
  hit_odds_t hit_probability{0};
  damage_t damage{0};
  damage_t total_damage{0};
  radiation_t radiation_dosage{0};
  radiation_t total_radiation{0};
  std::optional<armor_t> armor_reduced_to{std::nullopt};
  hit_count_t penetrations{0};
  armor_t effective_armor{0};
  armor_t defense{0};
  double penetration_probability{0.0};
  CriticalHitResult critical{};
  CollateralDamage collateral{};
};

export std::optional<ShipShotResult>
shoot_ship_to_ship(EntityManager& em, const Ship& attacker, Ship& target,
                   weapon_power_t cew_strength, weapon_range_t range,
                   bool ignore = false);
export std::optional<ShipShotResult>
shoot_planet_to_ship(EntityManager& em, const Race& race, Ship& target,
                     weapon_power_t strength);

export struct BombardResult {
  std::string ship_display{};
  std::string location_display{};
  player_t previous_sector_owner{0};
  sector_count_t sectors_destroyed{0};
  std::flat_set<player_t> nuked_players{};
};

export std::optional<BombardResult>
shoot_ship_to_planet(EntityManager& em, const Ship& attacker, Planet& target,
                     weapon_power_t strength, Coordinates target_sector,
                     SectorMap& sector_map, bool ignore = false,
                     guntype_t caliber = guntype_t::NONE);
export std::pair<hit_odds_t, weapon_range_t>
hit_odds(double range, double tech, damage_t fdam, bool fev, bool tev,
         speed_t fspeed, speed_t tspeed, ship_size_t body, guntype_t caliber,
         armor_t defense);

/// \brief Salvo saturation rule: every 5 hits reduce target effective armor by
/// 1 for that attack (help/fireformula.md).
export constexpr unsigned int HITS_PER_ARMOR_PENETRATION = 5;

/// \brief Inflection scalar for relative tech comparison in armor penetration
/// (help/fireformula.md).
export constexpr double TECH_PENETRATION_SCALE = 5.0;

/// \brief Computes per-armor-point penetration factor based on relative
/// technology (help/fireformula.md).
export double p_factor(double attacker_tech, double defender_tech);

export struct GroundAttackParams {
  const Race& attacker;
  const Race& defender;
  population_t attacker_force;
  PopulationType attacker_type;
  population_t defender_civ;
  population_t defender_mil;
  int attacker_defense_bonus;
  int defender_defense_bonus;
  double attacker_compatibility;
  double defender_compatibility;
};

export struct GroundAttackResult {
  double attack_strength{0.0};
  double defense_strength{0.0};
  population_t surviving_attackers{0};
  population_t surviving_defender_civ{0};
  population_t surviving_defender_mil{0};
  population_t attacker_casualties{0};
  population_t defender_civ_casualties{0};
  population_t defender_mil_casualties{0};
};

export [[nodiscard]] GroundAttackResult
ground_attack(const GroundAttackParams& params);

export struct MechAttackPeopleResult {
  std::string location_display{};
  std::string ship_display{};
  std::string defender_race_name{};
  player_t defender_player{0};
  Coordinates sector_coords{};
  std::string sector_condition{};
  weapon_power_t guns_fired{0};
  population_t initial_civ{0};
  population_t initial_mil{0};
  population_t surviving_civ{0};
  population_t surviving_mil{0};
  population_t civ_killed{0};
  population_t mil_killed{0};
  double attack_strength{0.0};
  double defense_strength{0.0};
};

export struct PeopleAttackMechResult {
  std::string location_display{};
  std::string attacker_race_name{};
  player_t attacker_player{0};
  bool mech_alive{true};
  std::string ship_display{};
  std::string ship_type_name{};
  Coordinates target_coords{};
  std::string sector_condition{};
  population_t attacker_civ{0};
  population_t attacker_mil{0};
  double attack_strength{0.0};
  double defense_strength{0.0};
  damage_t damage_inflicted{0};
  damage_t total_damage{0};
  CollateralDamage collateral{};
};

export struct MechDefendEngagementRound {
  player_t defender_player{0};
  governor_t defender_governor{0};
  MechAttackPeopleResult mech_attack{};
  std::optional<PeopleAttackMechResult> people_counterattack{std::nullopt};
};

export struct MechDefendResult {
  population_t surviving_people{0};
  std::vector<MechDefendEngagementRound> rounds{};
};

export MechDefendResult mech_defend(EntityManager& em,
                                    const Race& attacker_race,
                                    population_t* people, PopulationType what,
                                    const Planet& p, Coordinates target_coords,
                                    const Sector& s2);

export [[nodiscard]] MechAttackPeopleResult
mech_attack_people(EntityManager& em, Ship& ship, population_t* civ,
                   population_t* mil, const Race& race, const Race& alien,
                   const Sector& sect, bool ignore);

export [[nodiscard]] PeopleAttackMechResult
people_attack_mech(EntityManager& em, Ship& ship, population_t civ,
                   population_t mil, const Race& race, const Race& alien,
                   const Sector& sect, Coordinates target_coords);

export struct MineShipVictimReport {
  player_t victim_owner{0};
  governor_t victim_governor{0};
  ShipShotResult shot{};
};

export struct MineDetonationReport {
  std::string ship_display{};
  std::string orbit_display{};
  std::vector<MineShipVictimReport> ship_victims{};
  std::optional<BombardResult> planet_strike{std::nullopt};
};

/// \brief Simulates proximity triggering, detonation, ship collateral damage,
/// and orbital planetary bombardment for space mines.
/// \param ship Mine ship executing turn processing or manual detonation.
/// \param detonate Whether manual or forced detonation is triggered.
/// \param entity_manager Entity manager for spatial queries and mutations.
/// \return Structured detonation report if the mine detonated.
export std::optional<MineDetonationReport>
domine(Ship& ship, bool detonate, EntityManager& entity_manager);

/// \brief Checks whether an activated space mine's proximity fuse is tripped
/// by an enemy or non-allied ship within its trigger radius.
/// \param mine Mine ship evaluating proximity.
/// \param entity_manager Entity manager for spatial queries and alliance
/// lookups.
/// \return True if an enemy or non-allied ship is within trigger radius.
export bool check_mine_proximity_trigger(const Ship& mine,
                                         EntityManager& entity_manager);

/// \brief Detonates a mine against all valid victim ships in the same scope.
/// \param mine Mine ship delivering explosive payload.
/// \param entity_manager Entity manager for ship mutations.
/// \return Per-ship combat results for all damaged/attacked victims.
export std::vector<MineShipVictimReport>
detonate_mine_against_ships(Ship& mine, EntityManager& entity_manager);

/// \brief Detonates an orbital mine against the planetary surface.
/// \param mine Mine ship delivering orbital bombardment.
/// \param entity_manager Entity manager for planetary mutations.
/// \return Bombardment result if the mine was in planetary orbit.
export std::optional<BombardResult>
detonate_mine_against_planet(Ship& mine, EntityManager& entity_manager);

export enum class DetonateError {
  NotAMine,
  NotActivated,
  DockedOrLanded,
  DetonationFailed,
};

/// \brief Validates and manually detonates a single space mine.
export std::expected<MineDetonationReport, DetonateError>
detonate_ship_mine(EntityManager& entity_manager, Ship& ship);

export enum class PeacefulDockErrorReason {
  ShipIrradiated,
  ShipAlreadyDocked,
  CannotDockWithSelf,
  TargetNotFound,
  TargetNotCommandable,
  NotInSameScope,
  TargetAlreadyDocked,
  TooFarAway,
  InsufficientFuel,
};

export struct PeacefulDockError {
  PeacefulDockErrorReason reason{PeacefulDockErrorReason::TargetNotFound};
  bool abort_loop{false};
  std::string ship_display{};
  std::string target_display{};
  radiation_t radiation{0};
  double max_distance{0.0};
};

export struct PeacefulDockResult {
  std::string ship_display{};
  std::string target_display{};
  double distance{0.0};
  double fuel_cost{0.0};
  double initial_fuel{0.0};
  bool hyperdrive_deactivated{false};
};

/// \brief Validates and executes a peaceful ship-to-ship dock.
export std::expected<PeacefulDockResult, PeacefulDockError>
dock_single_ship(EntityManager& em, Ship& s, shipnum_t target_id,
                 player_t player, governor_t governor, bool god = false);

export struct EscortRetaliationEvent {
  player_t escort_owner{0};
  governor_t escort_governor{0};
  std::optional<ReactorOverloadEvent> overload{std::nullopt};
  std::optional<ShipShotResult> shot{std::nullopt};
};

export struct ShipCombatExchange {
  player_t shooter_owner{0};
  governor_t shooter_governor{0};
  player_t target_owner{0};
  governor_t target_governor{0};
  starnum_t star_id{0};
  std::optional<ReactorOverloadEvent> primary_overload{std::nullopt};
  bool primary_overload_fizzled{false};
  std::optional<ShipShotResult> primary_shot{std::nullopt};
  std::optional<ReactorOverloadEvent> retaliation_overload{std::nullopt};
  std::optional<ShipShotResult> retaliation_shot{std::nullopt};
  std::vector<EscortRetaliationEvent> escort_shots{};
};

/// \brief Executes pre-boarding defensive fire from a target ship against an
/// assaulting ship, including self-retaliation and escort retaliation.
export std::optional<ShipCombatExchange>
execute_defensive_fire(EntityManager& em, Ship& attacker, Ship& defender);

export struct BoardingOutcomeReport {
  PopulationType what{PopulationType::MIL};
  player_t old_defender_owner{0};
  governor_t old_defender_gov{0};
  ScopeLevel scope{ScopeLevel::LEVEL_STAR};
  starnum_t star_id{0};
  std::string attacker_display{};
  std::string target_display{};
  std::string attacker_orbit_display{};
  std::string target_orbit_display{};
  double attack_strength{0.0};
  double defense_strength{0.0};
  bool hyperdrive_deactivated{false};
  bool boobytrap_triggered{false};
  damage_t booby_damage{0};
  damage_t attacker_damage{0};
  damage_t attacker_total_damage{0};
  bool attacker_alive{true};
  damage_t defender_damage{0};
  damage_t defender_total_damage{0};
  bool defender_alive{true};
  bool captured{false};
  population_t surviving_boarders{0};
  population_t target_max_crew{0};
  bool defender_has_remaining_crew{false};
  bool attacker_has_remaining_crew{true};
  population_t attacker_casualties{0};
  population_t defender_civ_casualties{0};
  population_t defender_mil_casualties{0};
  CapturedShipsReport captured_ships{};
};

export enum class AssaultErrorReason {
  ShipIrradiated,
  PodsCannotAssault,
  ShipLandedOnCarrier,
  ShipAlreadyDocked,
  NoCrew,
  NoTroops,
  CannotAssaultSelf,
  TargetNotFound,
  NotInSameScope,
  CannotAssaultVonNeumann,
  TargetAlreadyLanded,
  TooFarAway,
  InsufficientFuel,
  IllegalBoarderCount,
  InsufficientUniverseAp,
  InsufficientStarAp,
  UnmoorFailed,
};

export struct AssaultError {
  AssaultErrorReason reason{AssaultErrorReason::TargetNotFound};
  bool abort_loop{false};
  std::string ship_display{};
  std::string target_display{};
  radiation_t radiation{0};
  double max_distance{0.0};
  population_t boarders{0};
};

export struct AssaultResult {
  std::string target_display{};
  double distance{0.0};
  double fuel_cost{0.0};
  double initial_fuel{0.0};
  bool abort_loop{false};
  std::optional<ShipCombatExchange> defensive_fire{std::nullopt};
  std::optional<BoardingOutcomeReport> boarding{std::nullopt};
};

/// \brief Validates and executes a hostile ship-to-ship boarding assault,
/// including AP deduction, pre-boarding defensive fire, and boarding combat.
export std::expected<AssaultResult, AssaultError>
assault_single_ship(EntityManager& em, Ship& s, shipnum_t target_id,
                    PopulationType what,
                    std::optional<population_t> requested_boarders,
                    player_t player, governor_t governor, bool god = false);

export enum class FireErrorReason {
  ShipIrradiated,
  CannotFireAtSelf,
  TargetNotFound,
  AfvNotLanded,
  AfvTargetNotLanded,
  LandedOnDifferentPlanets,
  NotAdjacentOnPlanet,
  NotEquippedForCew,
  NoCrystalMounted,
  InsufficientCewFuel,
  CewLandedOriginOrTarget,
  InsufficientUniverseAp,
  InsufficientStarAp,
  NoAttackStrength,
  IllegalAttack,
};

export struct FireError {
  FireErrorReason reason{FireErrorReason::NoAttackStrength};
  bool abort_loop{false};
  std::string ship_display{};
  std::string target_display{};
  weapon_power_t cew_strength{0};
  std::optional<weapon_power_t> clamped_strength{std::nullopt};
  bool clamped_is_laser{false};
};

export struct FireShipResult {
  std::optional<weapon_power_t> cew_strength{std::nullopt};
  std::optional<weapon_power_t> clamped_strength{std::nullopt};
  bool clamped_is_laser{false};
  ShipCombatExchange exchange{};
};

/// \brief Validates and executes conventional or laser weapon fire from a
/// single ship against a target ship, including target self-retaliation and
/// escort retaliation.
export std::expected<FireShipResult, FireError>
fire_single_ship(EntityManager& em, Ship& from, shipnum_t target_id,
                 std::optional<weapon_power_t> requested_strength,
                 player_t player, bool god = false);

/// \brief Validates and executes Confined Energy Weapon (CEW) fire from a
/// single ship against a target ship, including target self-retaliation and
/// escort retaliation.
export std::expected<FireShipResult, FireError>
cew_single_ship(EntityManager& em, Ship& from, shipnum_t target_id,
                player_t player, bool god = false);
