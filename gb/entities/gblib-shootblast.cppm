// SPDX-License-Identifier: Apache-2.0

export module gblib:shootblast;

import gb.entities;
import :misc;

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
