// SPDX-License-Identifier: Apache-2.0

/// \file move.cc
/// \brief Move population and assault aliens on target sector.

module;

import std;

module gblib;

namespace {

// TODO(C++26): Use std::inplace_vector when it lands in libc++ and make
// constexpr when P3372 (constexpr containers and adaptors) lands.
const std::flat_map<char, Coordinates> direction_mappings{
    {'1', {-1, 1}},  {'b', {-1, 1}},   // Southwest
    {'2', {0, 1}},   {'k', {0, 1}},    // South
    {'3', {1, 1}},   {'n', {1, 1}},    // Southeast
    {'4', {-1, 0}},  {'h', {-1, 0}},   // West
    {'6', {1, 0}},   {'l', {1, 0}},    // East
    {'7', {-1, -1}}, {'y', {-1, -1}},  // Northwest
    {'8', {0, -1}},  {'j', {0, -1}},   // North
    {'9', {1, -1}},  {'u', {1, -1}},   // Northeast
};

bool is_hostile_defending_afv(const GameObj& g, const Ship& ship,
                              Coordinates target_coords,
                              const Race& alien_race) {
  if (ship.owner() == g.player() || ship.type() != ShipType::OTYPE_AFV ||
      !ship.is_landed() || ship.retal_strength() == 0 ||
      ship.land_coords() != target_coords) {
    return false;
  }
  return !g.race->is_allied_with(ship.owner()) ||
         !alien_race.is_allied_with(g.player());
}

void resolve_afv_sector_engagement(const GameObj& g, Ship& ship,
                                   const Race& alien_race, governor_t alien_gov,
                                   const Sector& sect,
                                   Coordinates target_coords, population_t& civ,
                                   population_t& mil) {
  while ((civ + mil) > 0 && ship.retal_strength() > 0) {
    auto [short_buf, long_buf] = mech_attack_people(
        g.entity_manager, ship, &civ, &mil, alien_race, *g.race, sect, true);
    push_telegram(g.entity_manager, g.player(), g.governor(), long_buf);
    push_telegram(g.entity_manager, alien_race.Playernum, alien_gov, long_buf);
    if (civ + mil > 0) {
      auto [short_buf2, long_buf2] =
          people_attack_mech(g.entity_manager, ship, civ, mil, *g.race,
                             alien_race, sect, target_coords);
      push_telegram(g.entity_manager, g.player(), g.governor(), long_buf2);
      push_telegram(g.entity_manager, alien_race.Playernum, alien_gov,
                    long_buf2);
    }
  }
}

}  // namespace

/**
 * @brief Calculates the new coordinates based on the given direction.
 *
 * This function takes a Planet object, a direction character, and the current
 * coordinates as input. It calculates and returns the new coordinates based on
 * the given direction.
 *
 * @param planet The Planet object representing the game world.
 * @param direction The direction character indicating the movement direction.
 * @param from The current coordinates.
 * @return The new coordinates after the movement.
 */
Coordinates get_move(const Planet& planet, const char direction,
                     const Coordinates from) {
  if (const auto it = direction_mappings.find(direction);
      it != direction_mappings.end()) {
    return planet.wrap(from + it->second);
  }
  return from;
}

void mech_defend(const GameObj& g, population_t* people, PopulationType type,
                 const Planet& p, Coordinates target_coords, const Sector& s2) {
  population_t civ = (type == PopulationType::CIV) ? *people : 0;
  population_t mil = (type == PopulationType::CIV) ? 0 : *people;

  for (auto ship_handle :
       ShipList::on_planet(g.entity_manager, p.star_id(), p.planet_order())) {
    if (civ + mil == 0) break;
    Ship& ship = *ship_handle;
    const auto* alien_ptr = g.entity_manager.peek_race(ship.owner());
    if (!alien_ptr ||
        !is_hostile_defending_afv(g, ship, target_coords, *alien_ptr)) {
      continue;
    }
    const auto* star = g.entity_manager.peek_star(ship.storbits());
    const governor_t oldgov = star->governor(alien_ptr->Playernum);
    resolve_afv_sector_engagement(g, ship, *alien_ptr, oldgov, s2,
                                  target_coords, civ, mil);
  }
  *people = civ + mil;
}

namespace {

/// \brief Computes the combat strength of a mechanized AFV (ship) engaging
/// ground forces on a planetary sector.
///
/// Formula:
///   MECH_ATTACK * tech * retal_strength * ((armor + 1) / 100)
///   * ((100 - damage) / 100) * (1 + owner_sector_compatibility)
///   * morale_factor(owner_morale - opponent_morale)
constexpr double calculate_mech_combat_strength(
    const Ship& ship, const weapon_power_t retal_strength,
    const Race& mech_owner, const Race& opponent, const Sector& sect) {
  const double armor_factor =
      percent_to_fraction(static_cast<double>(ship.armor()) + 1.0);
  const double hull_integrity_factor =
      percent_to_fraction(100.0 - static_cast<double>(ship.damage()));
  return MECH_ATTACK * ship.tech() * static_cast<double>(retal_strength) *
         armor_factor * hull_integrity_factor *
         mech_owner.sector_combat_factor(sect) *
         morale_factor(
             static_cast<double>(mech_owner.morale - opponent.morale));
}

/// \brief Computes the combat strength of a sector's civilian and military
/// population engaging a mechanized AFV.
///
/// Formula:
///   ((10 * troops * fighters + civilians) / 100) * (tech / 100)
///   * (1 + garrison_sector_compatibility) * (1 + sector_defense_bonus)
///   * morale_factor(garrison_morale - opponent_morale)
constexpr double calculate_garrison_combat_strength(const population_t civ,
                                                    const population_t mil,
                                                    const Race& garrison_race,
                                                    const Race& opponent,
                                                    const Sector& sect) {
  const double weighted_personnel =
      MILITARY_COMBAT_MULTIPLIER * static_cast<double>(mil) *
          static_cast<double>(garrison_race.fighters) +
      static_cast<double>(civ);
  return percent_to_fraction(weighted_personnel) *
         percent_to_fraction(garrison_race.tech) *
         garrison_race.sector_combat_factor(sect) *
         sect.combat_defense_factor() *
         morale_factor(
             static_cast<double>(garrison_race.morale - opponent.morale));
}

}  // namespace

std::tuple<std::string, std::string>
mech_attack_people(EntityManager& em, Ship& ship, population_t* civ,
                   population_t* mil, const Race& race, const Race& alien,
                   const Sector& sect, bool ignore) {
  auto oldciv = *civ;
  auto oldmil = *mil;

  const auto strength = ship.retal_strength();
  const auto astrength =
      calculate_mech_combat_strength(ship, strength, race, alien, sect);
  const auto dstrength =
      calculate_garrison_combat_strength(oldciv, oldmil, alien, race, sect);

  if (ignore) {
    auto raw_ammo = static_cast<int>(std::log10(dstrength + 1.0)) - 1;
    auto ammo =
        std::min(static_cast<weapon_power_t>(std::max(raw_ammo, 0)), strength);
    ship.consume_destruct(ammo);
  } else {
    ship.consume_destruct(strength);
  }

  const double ratio = (dstrength > 0.0) ? std::min(1e6, astrength / dstrength)
                                         : (astrength > 0.0 ? 1e6 : 0.0);
  auto cas_civ = int_rand(0, round_rand(static_cast<double>(oldciv) * ratio));
  cas_civ = MIN(oldciv, cas_civ);
  auto cas_mil = int_rand(0, round_rand(static_cast<double>(oldmil) * ratio));
  cas_mil = MIN(oldmil, cas_mil);
  *civ -= cas_civ;
  *mil -= cas_mil;
  std::string short_msg =
      std::format("{}: {} {} {} [{}]\n", dispshiploc(em, ship), ship,
                  (*civ + *mil) ? "attacked" : "slaughtered", alien.name,
                  alien.Playernum.value);
  std::string long_msg =
      short_msg +
      std::format("\tBattle at {},{} {}: {} guns fired on {} civ/{} mil\n"
                  "\tAttack: {:.3f}   Defense: {:.3f}.\n"
                  "\t{} civ/{} mil killed.\n",
                  sect.get_x(), sect.get_y(), Desnames[sect.get_condition()],
                  strength, oldciv, oldmil, astrength, dstrength, cas_civ,
                  cas_mil);
  return std::make_tuple(short_msg, long_msg);
}

std::tuple<std::string, std::string>
people_attack_mech(EntityManager& em, Ship& ship, int civ, int mil,
                   const Race& race, const Race& alien, const Sector& sect,
                   Coordinates target_coords) {
  const auto strength = ship.retal_strength();

  const double dstrength =
      calculate_mech_combat_strength(ship, strength, alien, race, sect);
  const double astrength =
      calculate_garrison_combat_strength(civ, mil, race, alien, sect);
  auto raw_ammo = (int)std::log10((double)astrength + 1.0) - 1;
  auto ammo =
      std::min(strength, static_cast<weapon_power_t>(std::max(0, raw_ammo)));
  ship.consume_destruct(ammo);
  const double damage_ceiling =
      (dstrength > 0.0) ? std::min(1e6, 100.0 * astrength / dstrength)
                        : (astrength > 0.0 ? 100.0 : 0.0);
  auto damage = int_rand(0, round_rand(damage_ceiling));
  damage = std::min(100, damage);
  if (ship.apply_damage(damage).destroyed) {
    em.kill_ship(race.Playernum, ship);
  }
  auto [cas_civ, cas_mil, pdam, sdam] = do_collateral(ship, damage, alien.mass);
  std::string short_msg = std::format(
      "{}: {} [{}] {} {}\n", dispshiploc(em, ship), race.name,
      race.Playernum.value, ship.alive() ? "attacked" : "DESTROYED", ship);
  std::string long_msg =
      short_msg +
      std::format("\tBattle at {} {}: {} civ/{} mil assault {}\n"
                  "\tAttack: {:.3f}   Defense: {:.3f}.\n"
                  "\t{}% damage inflicted for a total of {}%\n"
                  "\t{} civ/{} mil killed   {} prim/{} sec guns knocked out\n",
                  target_coords, Desnames[sect.get_condition()], civ, mil,
                  ship.type_name(), astrength, dstrength, damage, ship.damage(),
                  cas_civ, cas_mil, pdam, sdam);
  return std::make_tuple(short_msg, long_msg);
}

GroundAttackResult ground_attack(const GroundAttackParams& p) {
  const double attacker_type_weight = (p.attacker_type == PopulationType::MIL)
                                          ? MILITARY_COMBAT_MULTIPLIER
                                          : 1.0;
  const double astrength =
      static_cast<double>(p.attacker_force) *
      static_cast<double>(p.attacker.fighters) * attacker_type_weight *
      (p.attacker_compatibility + 1.0) *
      (static_cast<double>(p.attacker_defense_bonus) + 1.0) *
      morale_factor(static_cast<double>(p.attacker.morale - p.defender.morale));

  const double dstrength =
      (static_cast<double>(p.defender_civ) +
       static_cast<double>(p.defender_mil) * MILITARY_COMBAT_MULTIPLIER) *
      static_cast<double>(p.defender.fighters) *
      (p.defender_compatibility + 1.0) *
      (static_cast<double>(p.defender_defense_bonus) + 1.0) *
      morale_factor(static_cast<double>(p.defender.morale - p.attacker.morale));

  const int attacker_effective_scale =
      static_cast<int>(p.attacker_force) *
      (p.attacker_type == PopulationType::MIL
           ? static_cast<int>(MILITARY_COMBAT_MULTIPLIER)
           : 1) *
      p.attacker.fighters;
  const int defender_effective_scale =
      static_cast<int>(p.defender_civ +
                       p.defender_mil *
                           static_cast<int>(MILITARY_COMBAT_MULTIPLIER)) *
      p.defender.fighters;
  const int casualty_scale =
      std::min(attacker_effective_scale, defender_effective_scale);

  const int attacker_divisor =
      (p.attacker_type == PopulationType::MIL)
          ? static_cast<int>(MILITARY_COMBAT_MULTIPLIER)
          : 1;
  population_t attacker_casualties = int_rand(
      0, round_rand(static_cast<double>(casualty_scale / attacker_divisor) *
                    dstrength / astrength));
  attacker_casualties = std::min(p.attacker_force, attacker_casualties);

  population_t defender_civ_casualties =
      int_rand(0, round_rand(static_cast<double>(casualty_scale) * astrength /
                             dstrength));
  defender_civ_casualties = std::min(p.defender_civ, defender_civ_casualties);

  population_t defender_mil_casualties =
      int_rand(0, round_rand(static_cast<double>(
                                 casualty_scale /
                                 static_cast<int>(MILITARY_COMBAT_MULTIPLIER)) *
                             astrength / dstrength));
  defender_mil_casualties = std::min(p.defender_mil, defender_mil_casualties);

  return GroundAttackResult{
      .attack_strength = astrength,
      .defense_strength = dstrength,
      .surviving_attackers = p.attacker_force - attacker_casualties,
      .surviving_defender_civ = p.defender_civ - defender_civ_casualties,
      .surviving_defender_mil = p.defender_mil - defender_mil_casualties,
      .attacker_casualties = attacker_casualties,
      .defender_civ_casualties = defender_civ_casualties,
      .defender_mil_casualties = defender_mil_casualties,
  };
}
