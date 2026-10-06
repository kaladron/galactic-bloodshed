// SPDX-License-Identifier: Apache-2.0

/// \file ground_combat.cc
/// \brief Ground population assault and mechanized AFV sector combat mechanics.

module;

import std;

module gb.mechanics;

namespace {

bool is_hostile_defending_afv(EntityManager& em, const Race& attacker_race,
                              const Ship& ship, Coordinates target_coords) {
  if (ship.owner() == attacker_race.Playernum ||
      ship.type() != ShipType::OTYPE_AFV || !ship.is_landed() ||
      ship.retal_strength() == 0 || ship.land_coords() != target_coords) {
    return false;
  }
  const auto& alien_race = *em.peek_race(ship.owner());
  return !attacker_race.is_allied_with(ship.owner()) ||
         !alien_race.is_allied_with(attacker_race.Playernum);
}

void resolve_afv_sector_engagement(
    EntityManager& em, const Race& attacker_race, Ship& ship,
    const Race& alien_race, governor_t alien_gov, const Sector& sect,
    Coordinates target_coords, population_t& civ, population_t& mil,
    std::vector<MechDefendEngagementRound>& rounds) {
  while ((civ + mil) > 0 && ship.retal_strength() > 0) {
    auto mech_attack = mech_attack_people(em, ship, &civ, &mil, alien_race,
                                          attacker_race, sect, true);
    std::optional<PeopleAttackMechResult> counterattack = std::nullopt;
    if (civ + mil > 0) {
      counterattack = people_attack_mech(em, ship, civ, mil, attacker_race,
                                         alien_race, sect, target_coords);
    }
    rounds.push_back(MechDefendEngagementRound{
        .defender_player = alien_race.Playernum,
        .defender_governor = alien_gov,
        .mech_attack = std::move(mech_attack),
        .people_counterattack = std::move(counterattack),
    });
  }
}

}  // namespace

MechDefendResult mech_defend(EntityManager& em, const Race& attacker_race,
                             population_t* people, PopulationType type,
                             const Planet& p, Coordinates target_coords,
                             const Sector& s2) {
  population_t civ = (type == PopulationType::CIV) ? *people : 0;
  population_t mil = (type == PopulationType::CIV) ? 0 : *people;
  std::vector<MechDefendEngagementRound> rounds;

  for (auto ship_handle :
       ShipList::on_planet(em, p.star_id(), p.planet_order())) {
    if (civ + mil == 0) break;
    Ship& ship = *ship_handle;
    if (!is_hostile_defending_afv(em, attacker_race, ship, target_coords)) {
      continue;
    }
    const auto& alien_race = *em.peek_race(ship.owner());
    const auto& star = *em.peek_star(ship.storbits());
    const governor_t oldgov = star.governor(alien_race.Playernum);
    resolve_afv_sector_engagement(em, attacker_race, ship, alien_race, oldgov,
                                  s2, target_coords, civ, mil, rounds);
  }
  *people = civ + mil;
  return MechDefendResult{
      .surviving_people = *people,
      .rounds = std::move(rounds),
  };
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

MechAttackPeopleResult mech_attack_people(EntityManager& em, Ship& ship,
                                          population_t* civ, population_t* mil,
                                          const Race& race, const Race& alien,
                                          const Sector& sect, bool ignore) {
  const auto oldciv = *civ;
  const auto oldmil = *mil;

  const auto strength = ship.retal_strength();
  const auto astrength =
      calculate_mech_combat_strength(ship, strength, race, alien, sect);
  const auto dstrength =
      calculate_garrison_combat_strength(oldciv, oldmil, alien, race, sect);

  if (ignore) {
    const auto raw_ammo = static_cast<int>(std::log10(dstrength + 1.0)) - 1;
    const auto ammo =
        std::min(static_cast<weapon_power_t>(std::max(raw_ammo, 0)), strength);
    ship.consume_destruct(ammo);
  } else {
    ship.consume_destruct(strength);
  }

  const double ratio = (dstrength > 0.0) ? std::min(1e6, astrength / dstrength)
                                         : (astrength > 0.0 ? 1e6 : 0.0);
  population_t cas_civ =
      int_rand(0, round_rand(static_cast<double>(oldciv) * ratio));
  cas_civ = std::min(oldciv, cas_civ);
  population_t cas_mil =
      int_rand(0, round_rand(static_cast<double>(oldmil) * ratio));
  cas_mil = std::min(oldmil, cas_mil);
  *civ -= cas_civ;
  *mil -= cas_mil;
  return MechAttackPeopleResult{
      .location_display = dispshiploc(em, ship),
      .ship_display = std::format("{}", ship),
      .defender_race_name = alien.name,
      .defender_player = alien.Playernum,
      .sector_coords = sect.coords(),
      .sector_condition = std::string(sect.condition_name()),
      .guns_fired = strength,
      .initial_civ = oldciv,
      .initial_mil = oldmil,
      .surviving_civ = *civ,
      .surviving_mil = *mil,
      .civ_killed = cas_civ,
      .mil_killed = cas_mil,
      .attack_strength = astrength,
      .defense_strength = dstrength,
  };
}

PeopleAttackMechResult people_attack_mech(EntityManager& em, Ship& ship,
                                          population_t civ, population_t mil,
                                          const Race& race, const Race& alien,
                                          const Sector& sect,
                                          Coordinates target_coords) {
  const auto strength = ship.retal_strength();

  const double dstrength =
      calculate_mech_combat_strength(ship, strength, alien, race, sect);
  const double astrength =
      calculate_garrison_combat_strength(civ, mil, race, alien, sect);
  const auto raw_ammo = static_cast<int>(std::log10(astrength + 1.0)) - 1;
  const auto ammo =
      std::min(strength, static_cast<weapon_power_t>(std::max(0, raw_ammo)));
  ship.consume_destruct(ammo);
  const double damage_ceiling =
      (dstrength > 0.0) ? std::min(1e6, 100.0 * astrength / dstrength)
                        : (astrength > 0.0 ? 100.0 : 0.0);
  auto damage = static_cast<damage_t>(
      std::min(100, int_rand(0, round_rand(damage_ceiling))));
  if (ship.apply_damage(damage).destroyed) {
    em.kill_ship(race.Playernum, ship);
  }
  const auto collateral = do_collateral(ship, damage, alien.mass);
  return PeopleAttackMechResult{
      .location_display = dispshiploc(em, ship),
      .attacker_race_name = race.name,
      .attacker_player = race.Playernum,
      .mech_alive = ship.alive(),
      .ship_display = std::format("{}", ship),
      .ship_type_name = std::string(ship.type_name()),
      .target_coords = target_coords,
      .sector_condition = std::string(sect.condition_name()),
      .attacker_civ = civ,
      .attacker_mil = mil,
      .attack_strength = astrength,
      .defense_strength = dstrength,
      .damage_inflicted = damage,
      .total_damage = ship.damage(),
      .collateral = collateral,
  };
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
