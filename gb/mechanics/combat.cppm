// SPDX-License-Identifier: Apache-2.0

/// \file combat.cppm
/// \brief Space combat, orbital bombardment, crystal overload, and ground/AFV
/// combat mechanics.

module;

export module gb.mechanics:combat;

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

export std::optional<ReactorOverloadEvent>
check_overload(EntityManager& entity_manager, Ship& ship, int cew,
               weapon_power_t* strength);

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
