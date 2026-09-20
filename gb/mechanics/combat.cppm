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
export void check_overload(EntityManager& entity_manager, Ship& ship, int cew,
                           weapon_power_t* strength);

// Damage, Short, Long
export std::optional<std::tuple<damage_t, std::string, std::string>>
shoot_ship_to_ship(EntityManager& em, const Ship& attacker, Ship& target,
                   weapon_power_t cew_strength, weapon_range_t range,
                   bool ignore = false);
export std::optional<std::tuple<damage_t, std::string, std::string>>
shoot_planet_to_ship(EntityManager& em, Race& race, Ship& target,
                     weapon_power_t strength);

export struct BombardResult {
  sector_count_t sectors_destroyed{0};
  PlayerVector<bool, MAXPLAYERS> nuked_players{};
  std::string short_message;
  std::string long_message;
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

/// \brief Collateral casualties and system damage inflicted on a target ship.
export struct CollateralDamage {
  population_t civilian_casualties{0};
  population_t military_casualties{0};
  gun_count_t primary_guns_lost{0};
  gun_count_t secondary_guns_lost{0};
};

export CollateralDamage do_collateral(Ship& ship, damage_t damage,
                                      double race_mass = 1.0);

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

export void mech_defend(const GameObj& g, population_t* people,
                        PopulationType what, const Planet& p,
                        Coordinates target_coords, const Sector& s2);

export std::tuple<std::string, std::string>
mech_attack_people(EntityManager& em, Ship& ship, population_t* civ,
                   population_t* mil, const Race& race, const Race& alien,
                   const Sector& sect, bool ignore);

export std::tuple<std::string, std::string>
people_attack_mech(EntityManager& em, Ship& ship, int civ, int mil,
                   const Race& race, const Race& alien, const Sector& sect,
                   Coordinates target_coords);
