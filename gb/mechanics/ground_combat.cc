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
  const double armor_factor = percent_to_fraction(ship.armor() + 1);
  const double hull_integrity_factor = percent_to_fraction(100 - ship.damage());
  return MECH_ATTACK * ship.tech() * static_cast<double>(retal_strength) *
         armor_factor * hull_integrity_factor *
         mech_owner.sector_combat_factor(sect) *
         morale_factor(mech_owner.morale - opponent.morale);
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
  const population_t weighted_personnel =
      MILITARY_COMBAT_MULTIPLIER * mil * garrison_race.fighters + civ;
  return percent_to_fraction(weighted_personnel) *
         percent_to_fraction(garrison_race.tech) *
         garrison_race.sector_combat_factor(sect) *
         sect.combat_defense_factor() *
         morale_factor(garrison_race.morale - opponent.morale);
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
    const auto ammo = std::clamp(raw_ammo, 0, strength);
    ship.consume_destruct(ammo);
  } else {
    ship.consume_destruct(strength);
  }

  const double ratio = (dstrength > 0.0) ? std::min(1e6, astrength / dstrength)
                                         : (astrength > 0.0 ? 1e6 : 0.0);
  population_t cas_civ = uniform_rand(0, round_rand(oldciv, ratio));
  cas_civ = std::min(oldciv, cas_civ);
  population_t cas_mil = uniform_rand(0, round_rand(oldmil, ratio));
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
  const auto ammo = std::clamp(raw_ammo, 0, strength);
  ship.consume_destruct(ammo);
  const double damage_ceiling =
      (dstrength > 0.0) ? std::min(1e6, 100.0 * astrength / dstrength)
                        : (astrength > 0.0 ? 100.0 : 0.0);
  const damage_t damage =
      std::min(100, uniform_rand(0, round_rand(damage_ceiling)));
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
  const population_t attacker_divisor =
      (p.attacker_type == PopulationType::MIL) ? MILITARY_COMBAT_MULTIPLIER : 1;
  const population_t attacker_effective_scale =
      p.attacker_force * attacker_divisor * p.attacker.fighters;
  const population_t defender_effective_scale =
      (p.defender_civ + p.defender_mil * MILITARY_COMBAT_MULTIPLIER) *
      p.defender.fighters;

  const double astrength =
      static_cast<double>(attacker_effective_scale) *
      (p.attacker_compatibility + 1.0) *
      (static_cast<double>(p.attacker_defense_bonus) + 1.0) *
      morale_factor(p.attacker.morale - p.defender.morale);

  const double dstrength =
      static_cast<double>(defender_effective_scale) *
      (p.defender_compatibility + 1.0) *
      (static_cast<double>(p.defender_defense_bonus) + 1.0) *
      morale_factor(p.defender.morale - p.attacker.morale);

  const population_t casualty_scale =
      std::min(attacker_effective_scale, defender_effective_scale);

  population_t attacker_casualties = uniform_rand(
      0, round_rand(casualty_scale / attacker_divisor, dstrength / astrength));
  attacker_casualties = std::min(p.attacker_force, attacker_casualties);

  population_t defender_civ_casualties =
      uniform_rand(0, round_rand(casualty_scale, astrength / dstrength));
  defender_civ_casualties = std::min(p.defender_civ, defender_civ_casualties);

  population_t defender_mil_casualties =
      uniform_rand(0, round_rand(casualty_scale / MILITARY_COMBAT_MULTIPLIER,
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

namespace {

std::optional<MilitiaError>
validate_militia_sector(EntityManager& em, starnum_t snum, planetnum_t pnum,
                        Coordinates coords, player_t player) {
  const auto& planet_peek = *em.peek_planet(snum, pnum);
  if (planet_peek.is_enslaved_to_foreign(player)) {
    return MilitiaError{.reason = MilitiaErrorReason::PlanetEnslaved};
  }
  if (!planet_peek.is_valid(coords)) {
    return MilitiaError{.reason = MilitiaErrorReason::InvalidCoordinates};
  }
  const auto& smap_peek = *em.peek_sectormap(snum, pnum);
  const auto& sect_peek = smap_peek.get(coords);
  if (sect_peek.get_owner() != player) {
    return MilitiaError{.reason = MilitiaErrorReason::SectorNotOwned};
  }
  return std::nullopt;
}

}  // namespace

std::expected<ArmSectorResult, MilitiaError>
arm_sector_militia(EntityManager& em, starnum_t snum, planetnum_t pnum,
                   Coordinates coords,
                   std::optional<population_t> requested_amount,
                   player_t player, governor_t governor) {
  if (auto err = validate_militia_sector(em, snum, pnum, coords, player)) {
    return std::unexpected(*err);
  }

  const auto& planet_peek = *em.peek_planet(snum, pnum);
  const auto& smap_peek = *em.peek_sectormap(snum, pnum);
  const auto& sect_peek = smap_peek.get(coords);
  const auto& race_peek = *em.peek_race(player);

  const population_t max_allowed =
      std::min(sect_peek.get_popn(),
               static_cast<population_t>(planet_peek.info(player).destruct *
                                         (sect_peek.get_mobilization() + 1)));
  const population_t amount =
      std::min(requested_amount.value_or(max_allowed), max_allowed);
  if (amount <= 0) {
    return std::unexpected(
        MilitiaError{.reason = MilitiaErrorReason::NoCiviliansToArm});
  }

  const money_t enlist_cost = race_peek.fighters * amount;
  if (enlist_cost > race_peek.governor(governor).money) {
    return std::unexpected(MilitiaError{
        .reason = MilitiaErrorReason::InsufficientMoney,
        .required_money = enlist_cost,
        .requested_troops = amount,
    });
  }

  em.mutate_race(player, [&](Race& race_mut) {
    race_mut.governor(governor).money -= enlist_cost;
  });

  const resource_t cost =
      std::max<resource_t>(1, amount / (sect_peek.get_mobilization() + 1));
  population_t final_popn = 0;
  population_t final_troops = 0;

  em.mutate_sectormap(snum, pnum, [&](SectorMap& smap) {
    auto& sect = smap.get(coords);
    sect.set_troops(sect.get_troops() + amount);
    sect.subtract_popn(amount);
    final_popn = sect.get_popn();
    final_troops = sect.get_troops();
  });

  em.mutate_planet(snum, pnum, [&](Planet& planet) {
    planet.popn() -= amount;
    planet.info(player).popn -= amount;
    planet.troops() += amount;
    planet.info(player).troops += amount;
    planet.info(player).destruct -= cost;
  });

  return ArmSectorResult{
      .armed = amount,
      .destruct_cost = cost,
      .enlist_cost = enlist_cost,
      .final_popn = final_popn,
      .final_troops = final_troops,
  };
}

std::expected<DisarmSectorResult, MilitiaError> disarm_sector_militia(
    EntityManager& em, starnum_t snum, planetnum_t pnum, Coordinates coords,
    std::optional<population_t> requested_amount, player_t player) {
  if (auto err = validate_militia_sector(em, snum, pnum, coords, player)) {
    return std::unexpected(*err);
  }

  const auto& smap_peek = *em.peek_sectormap(snum, pnum);
  const auto& sect_peek = smap_peek.get(coords);

  const population_t amount =
      requested_amount ? std::clamp<population_t>(*requested_amount, 0,
                                                  sect_peek.get_troops())
                       : sect_peek.get_troops();

  population_t final_popn = 0;
  population_t final_troops = 0;

  em.mutate_sectormap(snum, pnum, [&](SectorMap& smap) {
    auto& sect = smap.get(coords);
    sect.add_popn(amount);
    sect.set_troops(sect.get_troops() - amount);
    final_popn = sect.get_popn();
    final_troops = sect.get_troops();
  });

  em.mutate_planet(snum, pnum, [&](Planet& planet) {
    planet.popn() += amount;
    planet.troops() -= amount;
    planet.info(player).popn += amount;
    planet.info(player).troops -= amount;
  });

  return DisarmSectorResult{
      .disarmed = amount,
      .final_popn = final_popn,
      .final_troops = final_troops,
  };
}

SectorGroundAssaultResult
execute_sector_ground_assault(EntityManager& em, starnum_t snum,
                              planetnum_t pnum, player_t attacker_player,
                              Coordinates from_coords, Coordinates to_coords,
                              population_t people, PopulationType what) {
  const auto& star = *em.peek_star(snum);
  const auto& smap_peek = *em.peek_sectormap(snum, pnum);
  const player_t defender_player = smap_peek.get(to_coords).get_owner();

  Race race = *em.peek_race(attacker_player);
  Race alien = *em.peek_race(defender_player);

  em.mutate_star(snum, [&](Star& s) { s.record_ground_assault(race, alien); });

  alien.increase_translation(attacker_player);
  race.increase_translation(defender_player);

  SectorGroundAssaultResult res{
      .star_name = star.get_name(),
      .planet_name = star.get_planet_name(pnum),
      .attacker_player = attacker_player,
      .attacker_race_name = race.name,
      .defender_player = defender_player,
      .defender_governor = star.governor(defender_player),
      .defender_race_name = alien.name,
      .from_coords = from_coords,
      .to_coords = to_coords,
      .attacker_type = what,
      .initial_attacker_popn = people,
  };

  em.mutate_planet_and_sectors(
      snum, pnum, [&](Planet& planet, SectorMap& smap) {
        auto& sect = smap.get(from_coords);
        auto& sect2 = smap.get(to_coords);

        res.from_condition_symbol = sect.condition_symbol();
        res.to_condition_symbol = sect2.condition_symbol();
        res.initial_defender_civ = sect2.get_popn();
        res.initial_defender_mil = sect2.get_troops();

        if (what == PopulationType::CIV) {
          sect.subtract_popn(people);
        } else {
          sect.subtract_troops(people);
        }

        const auto outcome = ground_attack({
            .attacker = race,
            .defender = alien,
            .attacker_force = people,
            .attacker_type = what,
            .defender_civ = res.initial_defender_civ,
            .defender_mil = res.initial_defender_mil,
            .attacker_defense_bonus = sect.defense_bonus(),
            .defender_defense_bonus = sect2.defense_bonus(),
            .attacker_compatibility = race.sector_compatibility(sect),
            .defender_compatibility = alien.sector_compatibility(sect2),
        });
        const population_t surviving = outcome.surviving_attackers;
        res.surviving_attackers = surviving;
        res.attack_strength = outcome.attack_strength;
        res.defense_strength = outcome.defense_strength;
        res.attacker_casualties = outcome.attacker_casualties;
        res.defender_civ_casualties = outcome.defender_civ_casualties;
        res.defender_mil_casualties = outcome.defender_mil_casualties;

        sect2.set_popn_exact(outcome.surviving_defender_civ);
        sect2.set_troops(outcome.surviving_defender_mil);

        if (sect2.is_empty()) {
          population_t absorbed = 0;
          if (race.absorb) {
            res.attacker_absorbed_bodies = true;
            absorbed = uniform_rand(0, res.initial_defender_civ +
                                           res.initial_defender_mil);
            res.absorbed_bodies = absorbed;
          }
          if (what == PopulationType::CIV) {
            sect2.set_popn_exact(surviving + absorbed);
          } else {
            sect2.set_popn_exact(absorbed);
            sect2.set_troops(surviving);
          }
          if (!sect2.is_empty()) {
            sect2.claim(attacker_player);
            race.adjust_morale(alien, alien.fighters);
          } else {
            sect2.clear_owner();
          }
        } else {
          if (alien.absorb) {
            res.defender_absorbed_bodies = true;
            const population_t absorbed =
                uniform_rand(0, res.initial_attacker_popn - surviving);
            res.absorbed_bodies = absorbed;
            sect2.add_popn(absorbed);
          }
          if (what == PopulationType::CIV) {
            sect.add_popn(surviving);
          } else {
            sect.add_troops(surviving);
          }
          alien.adjust_morale(race, race.fighters);
        }

        res.attacker_won = (sect2.get_owner() == attacker_player);
        res.all_origin_forces_wiped =
            (sect.get_popn() + sect.get_troops() + surviving == 0);
        if (res.all_origin_forces_wiped) {
          race.increase_translation(defender_player);
        }
        if (surviving == 0) {
          alien.increase_translation(attacker_player);
        }

        sect.clear_owner_if_empty();
        res.stop_movement = (!res.attacker_won || sect2.is_empty());
        planet.sync_demographics(smap);
      });

  em.mutate_race(attacker_player, [&](Race& r) { r = race; });
  em.mutate_race(defender_player, [&](Race& a) { a = alien; });
  return res;
}
