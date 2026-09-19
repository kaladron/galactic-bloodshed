// SPDX-License-Identifier: Apache-2.0

export module gblib:move;

import :planet;
import :sector;
import :services;
import :ships;
import :types;

export Coordinates get_move(const Planet& planet, char direction,
                            Coordinates from);

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
