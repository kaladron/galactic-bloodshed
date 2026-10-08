// SPDX-License-Identifier: Apache-2.0

/// \file boarding.cc
/// \brief Peaceful docking, pre-boarding defensive fire, and ship-to-ship
/// boarding combat mechanics.

module;

import std;

module gb.mechanics;

namespace {

population_t compute_boarders(const Ship& s, const Ship& s2,
                              PopulationType what,
                              std::optional<population_t> requested_boarders) {
  population_t boarders = (what == PopulationType::CIV) ? s.popn() : s.troops();
  if (requested_boarders) {
    boarders = std::min(boarders, *requested_boarders);
  }
  return std::min(boarders, s2.max_crew());
}

bool has_boarding_population(const Ship& s, PopulationType what) noexcept {
  return (what == PopulationType::CIV) ? (s.popn() > 0) : (s.troops() > 0);
}

void remove_ship_boarders(Ship& s, PopulationType what, population_t count,
                          double race_mass) {
  if (what == PopulationType::MIL) {
    s.remove_troops(count, race_mass);
  } else {
    s.remove_popn(count, race_mass);
  }
}

void add_ship_boarders(Ship& s, PopulationType what, population_t count,
                       double race_mass) {
  if (what == PopulationType::MIL) {
    s.add_troops(count, race_mass);
  } else {
    s.add_popn(count, race_mass);
  }
}

bool maneuver_ship_to_target(Ship& s, const Ship& s2, double fuel) {
  s.consume_fuel(fuel);
  s.set_coordinates(s2.coordinates() +
                    SystemCoordinates{static_cast<double>(int_rand(-1, 1)),
                                      static_cast<double>(int_rand(-1, 1))});
  if (s.hyper_drive().on) {
    s.hyper_drive().on = 0;
    return true;
  }
  return false;
}

bool is_eligible_escort(const Ship& escort, shipnum_t protected_id,
                        shipnum_t target_id) {
  if (!escort.active() || !escort.protect().on) {
    return false;
  }
  if (escort.protect().ship != protected_id) {
    return false;
  }
  return escort.number() != protected_id && escort.number() != target_id;
}

void resolve_defensive_self_retaliation(EntityManager& em, Ship& attacker,
                                        Ship& defender, weapon_power_t retal,
                                        damage_t damage,
                                        ShipCombatExchange& exchange) {
  if (retal <= 0 || damage <= 0 || !attacker.protect().retaliate) {
    return;
  }
  auto [retal_strength, overload] = check_overload(em, attacker, 0, retal);
  if (overload) {
    exchange.retaliation_overload = overload;
    return;
  }
  exchange.retaliation_shot =
      shoot_ship_to_ship(em, attacker, defender, retal_strength, 0, true);
  if (exchange.retaliation_shot) {
    attacker.consume_weapon_resources(retal_strength);
  }
}

std::optional<EscortRetaliationEvent>
execute_escort_shot(EntityManager& em, Ship& escort, Ship& defender) {
  auto [strength, overload] =
      check_overload(em, escort, 0, escort.check_retal_strength());
  if (overload) {
    return EscortRetaliationEvent{
        .escort_owner = escort.owner(),
        .escort_governor = escort.governor(),
        .overload = overload,
    };
  }
  auto shot = shoot_ship_to_ship(em, escort, defender, strength, 0);
  if (!shot) {
    return std::nullopt;
  }
  escort.consume_weapon_resources(strength);
  return EscortRetaliationEvent{
      .escort_owner = escort.owner(),
      .escort_governor = escort.governor(),
      .shot = shot,
  };
}

void resolve_defensive_escort_retaliation(EntityManager& em,
                                          const Ship& attacker, Ship& defender,
                                          damage_t damage,
                                          ShipCombatExchange& exchange) {
  if (damage <= 0 || !defender.alive()) {
    return;
  }
  if (attacker.whatorbits() != ScopeLevel::LEVEL_STAR &&
      attacker.whatorbits() != ScopeLevel::LEVEL_PLAN) {
    return;
  }

  ShipList shiplist =
      (attacker.whatorbits() == ScopeLevel::LEVEL_STAR)
          ? ShipList::in_star(em, attacker.storbits())
          : ShipList::on_planet(em, attacker.storbits(), attacker.pnumorbits());
  for (auto ship_handle : shiplist) {
    if (!defender.alive()) {
      break;
    }
    Ship& escort = *ship_handle;
    if (!is_eligible_escort(escort, attacker.number(), defender.number())) {
      continue;
    }
    if (auto event = execute_escort_shot(em, escort, defender)) {
      exchange.escort_shots.push_back(*event);
    }
  }
}

void apply_boarding_casualties(EntityManager& em, Ship& s, Ship& s2,
                               Race& alien, double bstrength, double b2strength,
                               player_t attacker_player,
                               BoardingOutcomeReport& report) {
  const population_t casualty_scale =
      std::min(report.surviving_boarders, s2.troops() + s2.popn());

  if (b2strength > 0.0) {
    report.attacker_casualties =
        std::min(report.surviving_boarders,
                 static_cast<population_t>(int_rand(
                     0, round_rand(static_cast<double>(casualty_scale) *
                                   (b2strength + 1.0) / (bstrength + 1.0)))));
    report.surviving_boarders -= report.attacker_casualties;

    report.attacker_damage = static_cast<damage_t>(
        std::min(100, int_rand(0, round_rand(25.0 * (b2strength + 1.0) /
                                             (bstrength + 1.0)))));
    if (s.apply_damage(report.attacker_damage).destroyed) {
      em.kill_ship(attacker_player, s);
    }

    report.defender_civ_casualties = std::min(
        s2.popn(), static_cast<population_t>(int_rand(
                       0, round_rand(static_cast<double>(casualty_scale) *
                                     (bstrength + 1.0) / (b2strength + 1.0)))));
    report.defender_mil_casualties =
        std::min(s2.troops(),
                 static_cast<population_t>(int_rand(
                     0, round_rand(static_cast<double>(casualty_scale) *
                                   (bstrength + 1.0) / (b2strength + 1.0)))));
    s2.remove_popn(report.defender_civ_casualties, alien.mass);
    s2.remove_troops(report.defender_mil_casualties, alien.mass);

    report.defender_damage = static_cast<damage_t>(
        std::min(100, int_rand(0, round_rand(25.0 * (bstrength + 1.0) /
                                             (b2strength + 1.0)))));
    if (s2.apply_damage(report.defender_damage).destroyed) {
      em.kill_ship(attacker_player, s2);
    }
    return;
  }

  s2.clear_crew(alien.mass);
  if (report.boobytrap_triggered) {
    report.booby_damage = static_cast<damage_t>(
        std::min<std::int64_t>(100, long_rand(0, 10 * s2.destruct())));
    report.attacker_damage += report.booby_damage;
    if (s.apply_damage(report.booby_damage).destroyed) {
      em.kill_ship(attacker_player, s);
    }
  }
}

void finalize_boarding_ownership_and_morale(EntityManager& em, Ship& s,
                                            Ship& s2, Race& race, Race& alien,
                                            PopulationType what,
                                            player_t attacker_player,
                                            BoardingOutcomeReport& report) {
  const bool defender_wiped_out = (s2.popn() + s2.troops() == 0);
  const bool both_ships_alive = s.alive() && s2.alive();

  if (defender_wiped_out && both_ships_alive) {
    s.moor_together(s2);
    s2.owner() = s.owner();
    s2.governor() = s.governor();
    add_ship_boarders(s2, what, report.surviving_boarders, race.mass);
    if (report.defender_civ_casualties + report.defender_mil_casualties > 0) {
      race.adjust_morale(alien, static_cast<int>(s2.build_cost()));
    }
    report.captured = true;
    report.captured_ships = capture_stuff(em, s2);
  } else if (s2.alive()) {
    if (s.alive()) {
      add_ship_boarders(s, what, report.surviving_boarders, race.mass);
    }
    alien.adjust_morale(race, static_cast<int>(race.fighters));
  }

  alien.increase_translation(attacker_player);
  race.increase_translation(report.old_defender_owner);

  if (!report.surviving_boarders && !defender_wiped_out) {
    alien.increase_translation(attacker_player, 25);
  }
  if (report.captured) {
    race.increase_translation(report.old_defender_owner, 25);
  }
}

std::expected<BoardingOutcomeReport, AssaultErrorReason>
resolve_boarding_combat(EntityManager& em, Ship& s, Ship& s2,
                        PopulationType what,
                        std::optional<population_t> requested_boarders,
                        double fuel) {
  BoardingOutcomeReport report{};
  report.what = what;
  const player_t attacker_player = s.owner();

  bool unmoor_failed = false;
  em.mutate_race(attacker_player, [&](Race& race) {
    em.mutate_race(s2.owner(), [&](Race& alien) {
      report.surviving_boarders =
          compute_boarders(s, s2, what, requested_boarders);
      report.old_defender_owner = s2.owner();
      report.old_defender_gov = s2.governor();
      report.scope = s.whatorbits();
      report.star_id = s.storbits();
      report.target_max_crew = s2.max_crew();
      report.boobytrap_triggered = (!s2.max_crew() && s2.destruct() > 0);

      remove_ship_boarders(s, what, report.surviving_boarders, race.mass);

      report.attack_strength =
          report.surviving_boarders *
          (what == PopulationType::MIL ? 10 * race.fighters : 1) * .01 *
          race.tech *
          morale_factor(static_cast<double>(race.morale - alien.morale));
      report.defense_strength =
          (s2.popn() + 10 * s2.troops() * alien.fighters) * .01 * alien.tech *
          morale_factor(static_cast<double>(alien.morale - race.morale));

      report.hyperdrive_deactivated = maneuver_ship_to_target(s, s2, fuel);

      if (s2.docked()) {
        if (auto res = em.unmoor_ships(s2.number()); !res) {
          unmoor_failed = true;
          return;
        }
      }

      apply_boarding_casualties(em, s, s2, alien, report.attack_strength,
                                report.defense_strength, attacker_player,
                                report);
      finalize_boarding_ownership_and_morale(em, s, s2, race, alien, what,
                                             attacker_player, report);

      report.attacker_display = std::format("{}", s);
      report.target_display = std::format("{}", s2);
      report.attacker_orbit_display = prin_ship_orbits(em, s);
      report.target_orbit_display = prin_ship_orbits(em, s2);
      report.attacker_total_damage = s.damage();
      report.attacker_alive = s.alive();
      report.defender_total_damage = s2.damage();
      report.defender_alive = s2.alive();
      report.defender_has_remaining_crew = (s2.popn() + s2.troops() > 0);
      report.attacker_has_remaining_crew = (s.popn() + s.troops() > 0);
    });
  });

  if (unmoor_failed) {
    return std::unexpected(AssaultErrorReason::UnmoorFailed);
  }
  return report;
}

std::optional<AssaultError> validate_assault_attacker(const Ship& s,
                                                      PopulationType what,
                                                      shipnum_t target_id) {
  if (!s.active()) {
    return AssaultError{
        .reason = AssaultErrorReason::ShipIrradiated,
        .ship_display = std::format("{}", s),
        .radiation = s.rad(),
    };
  }
  if (s.type() == ShipType::STYPE_POD) {
    return AssaultError{.reason = AssaultErrorReason::PodsCannotAssault};
  }
  if (s.whatorbits() == ScopeLevel::LEVEL_SHIP) {
    return AssaultError{.reason = AssaultErrorReason::ShipLandedOnCarrier};
  }
  if (s.docked()) {
    return AssaultError{.reason = AssaultErrorReason::ShipAlreadyDocked};
  }
  if (!has_boarding_population(s, what)) {
    return AssaultError{
        .reason = (what == PopulationType::CIV) ? AssaultErrorReason::NoCrew
                                                : AssaultErrorReason::NoTroops,
    };
  }
  if (s.number() == target_id) {
    return AssaultError{.reason = AssaultErrorReason::CannotAssaultSelf};
  }
  return std::nullopt;
}

std::optional<AssaultError>
validate_assault_target(const Ship& s, const Ship& s2, PopulationType what,
                        std::optional<population_t> requested_boarders) {
  if (s.whatorbits() != s2.whatorbits()) {
    return AssaultError{.reason = AssaultErrorReason::NotInSameScope};
  }
  if (s2.type() == ShipType::OTYPE_VN) {
    return AssaultError{.reason = AssaultErrorReason::CannotAssaultVonNeumann};
  }
  if (s2.is_landed()) {
    return AssaultError{
        .reason = AssaultErrorReason::TargetAlreadyLanded,
        .target_display = std::format("{}", s2),
    };
  }
  if (s2.coordinates().distance_to(s.coordinates()) > DIST_TO_DOCK) {
    return AssaultError{
        .reason = AssaultErrorReason::TooFarAway,
        .ship_display = std::format("{}", s),
        .target_display = std::format("{}", s2),
        .max_distance = DIST_TO_DOCK,
    };
  }
  if (s.docking_fuel_cost(s2, true) > s.fuel()) {
    return AssaultError{.reason = AssaultErrorReason::InsufficientFuel};
  }
  const population_t boarders =
      compute_boarders(s, s2, what, requested_boarders);
  if (s2.max_crew() && boarders <= 0) {
    return AssaultError{
        .reason = AssaultErrorReason::IllegalBoarderCount,
        .boarders = boarders,
    };
  }
  return std::nullopt;
}

std::optional<AssaultError> deduct_assault_ap(EntityManager& em, const Ship& s,
                                              player_t player, bool god) {
  if (god) {
    return std::nullopt;
  }
  if (s.whatorbits() == ScopeLevel::LEVEL_UNIV) {
    if (em.peek_universe()->get_AP(player) < 1) {
      return AssaultError{
          .reason = AssaultErrorReason::InsufficientUniverseAp,
      };
    }
    em.mutate_universe([&](universe_struct& u) { u.deduct_AP(player, 1); });
    return std::nullopt;
  }
  if (em.peek_star(s.storbits())->AP(player) < 1) {
    return AssaultError{
        .reason = AssaultErrorReason::InsufficientStarAp,
    };
  }
  em.mutate_star(s.storbits(), [&](Star& st) { st.AP(player) -= 1; });
  return std::nullopt;
}

}  // namespace

std::expected<PeacefulDockResult, PeacefulDockError>
dock_single_ship(EntityManager& em, Ship& s, shipnum_t target_id,
                 player_t player, governor_t governor, bool god) {
  if (!s.active()) {
    return std::unexpected(PeacefulDockError{
        .reason = PeacefulDockErrorReason::ShipIrradiated,
        .ship_display = std::format("{}", s),
        .radiation = s.rad(),
    });
  }
  if (s.docked()) {
    return std::unexpected(PeacefulDockError{
        .reason = PeacefulDockErrorReason::ShipAlreadyDocked,
        .ship_display = std::format("{}", s),
    });
  }
  if (s.number() == target_id) {
    return std::unexpected(PeacefulDockError{
        .reason = PeacefulDockErrorReason::CannotDockWithSelf,
    });
  }

  const Ship* s2 = nullptr;
  try {
    s2 = em.peek_ship(target_id);
  } catch (const EntityNotFoundError&) {
    return std::unexpected(PeacefulDockError{
        .reason = PeacefulDockErrorReason::TargetNotFound,
        .abort_loop = true,
    });
  }

  if (!validate_commandable(*s2, player, governor, god)) {
    return std::unexpected(PeacefulDockError{
        .reason = PeacefulDockErrorReason::TargetNotCommandable,
        .abort_loop = true,
    });
  }
  if (s.whatorbits() != s2->whatorbits()) {
    return std::unexpected(PeacefulDockError{
        .reason = PeacefulDockErrorReason::NotInSameScope,
    });
  }
  if (s2->docked()) {
    return std::unexpected(PeacefulDockError{
        .reason = PeacefulDockErrorReason::TargetAlreadyDocked,
        .abort_loop = true,
        .target_display = std::format("{}", *s2),
    });
  }

  const double dist = s2->coordinates().distance_to(s.coordinates());
  const double fuel = s.docking_fuel_cost(*s2, false);

  if (dist > DIST_TO_DOCK) {
    return std::unexpected(PeacefulDockError{
        .reason = PeacefulDockErrorReason::TooFarAway,
        .ship_display = std::format("{}", s),
        .target_display = std::format("{}", *s2),
        .max_distance = DIST_TO_DOCK,
    });
  }
  if (fuel > s.fuel()) {
    return std::unexpected(PeacefulDockError{
        .reason = PeacefulDockErrorReason::InsufficientFuel,
    });
  }

  PeacefulDockResult result{
      .ship_display = std::format("{}", s),
      .target_display = std::format("{}", *s2),
      .distance = dist,
      .fuel_cost = fuel,
      .initial_fuel = s.fuel(),
  };

  em.mutate_ship(target_id, [&](Ship& target) {
    result.hyperdrive_deactivated = maneuver_ship_to_target(s, target, fuel);
    s.moor_together(target);
    s.notified() = target.notified() = 0;
  });

  return result;
}

std::optional<ShipCombatExchange>
execute_defensive_fire(EntityManager& em, Ship& attacker, Ship& defender) {
  if (!defender.alive() || !defender.active()) {
    return std::nullopt;
  }

  const weapon_power_t initial_strength = defender.check_retal_strength();
  if (initial_strength <= 0) {
    return std::nullopt;
  }

  ShipCombatExchange exchange{
      .shooter_owner = defender.owner(),
      .shooter_governor = defender.governor(),
      .target_owner = attacker.owner(),
      .target_governor = attacker.governor(),
      .star_id = defender.storbits(),
  };

  auto [strength, overload] = check_overload(em, defender, 0, initial_strength);
  if (overload) {
    exchange.primary_overload = overload;
    exchange.primary_overload_fizzled = true;
    return exchange;
  }

  const auto retal = attacker.check_retal_strength();
  exchange.primary_shot =
      shoot_ship_to_ship(em, defender, attacker, strength, 0);
  if (!exchange.primary_shot) {
    return std::nullopt;
  }

  defender.consume_weapon_resources(strength);
  const damage_t damage = exchange.primary_shot->damage;
  resolve_defensive_self_retaliation(em, attacker, defender, retal, damage,
                                     exchange);
  resolve_defensive_escort_retaliation(em, attacker, defender, damage,
                                       exchange);
  return exchange;
}

std::expected<AssaultResult, AssaultError>
assault_single_ship(EntityManager& em, Ship& s, shipnum_t target_id,
                    PopulationType what,
                    std::optional<population_t> requested_boarders,
                    player_t player, governor_t, bool god) {
  if (auto attacker_err = validate_assault_attacker(s, what, target_id)) {
    return std::unexpected(*attacker_err);
  }

  const Ship* s2_peek = nullptr;
  try {
    s2_peek = em.peek_ship(target_id);
  } catch (const EntityNotFoundError&) {
    return std::unexpected(AssaultError{
        .reason = AssaultErrorReason::TargetNotFound,
        .abort_loop = true,
    });
  }

  if (auto target_err =
          validate_assault_target(s, *s2_peek, what, requested_boarders)) {
    return std::unexpected(*target_err);
  }

  if (auto ap_err = deduct_assault_ap(em, s, player, god)) {
    return std::unexpected(*ap_err);
  }

  const double dist = s2_peek->coordinates().distance_to(s.coordinates());
  const double fuel = s.docking_fuel_cost(*s2_peek, true);

  AssaultResult result{
      .target_display = std::format("{}", *s2_peek),
      .distance = dist,
      .fuel_cost = fuel,
      .initial_fuel = s.fuel(),
  };

  std::optional<AssaultErrorReason> boarding_err;
  em.mutate_ship(target_id, [&](Ship& s2) {
    result.defensive_fire = execute_defensive_fire(em, s, s2);
    if (!s2.alive()) {
      result.abort_loop = true;
      return;
    }
    if (!s.alive() || !has_boarding_population(s, what)) {
      return;
    }

    auto boarding_res =
        resolve_boarding_combat(em, s, s2, what, requested_boarders, fuel);
    if (!boarding_res) {
      boarding_err = boarding_res.error();
      return;
    }
    result.boarding = *boarding_res;
    s.notified() = s2.notified() = 0;
  });

  if (boarding_err) {
    return std::unexpected(AssaultError{.reason = *boarding_err});
  }

  return result;
}
