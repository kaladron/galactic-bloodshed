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
  s.set_coordinates(
      s2.coordinates() +
      SystemCoordinates{static_cast<double>(uniform_rand(-1, 1)),
                        static_cast<double>(uniform_rand(-1, 1))});
  if (s.hyper_drive().on) {
    s.hyper_drive().on = 0;
    return true;
  }
  return false;
}

void apply_boarding_casualties(EntityManager& em, Ship& s, Ship& s2,
                               Race& alien, double bstrength, double b2strength,
                               player_t attacker_player,
                               BoardingOutcomeReport& report) {
  const population_t casualty_scale =
      std::min(report.surviving_boarders, s2.troops() + s2.popn());

  if (b2strength > 0.0) {
    report.attacker_casualties = std::min(
        report.surviving_boarders,
        uniform_rand(0, round_rand(casualty_scale,
                                   (b2strength + 1.0) / (bstrength + 1.0))));
    report.surviving_boarders -= report.attacker_casualties;

    report.attacker_damage =
        std::min(100, uniform_rand(0, round_rand(25.0 * (b2strength + 1.0) /
                                                 (bstrength + 1.0))));
    if (s.apply_damage(report.attacker_damage).destroyed) {
      em.kill_ship(attacker_player, s);
    }

    report.defender_civ_casualties = std::min(
        s2.popn(),
        uniform_rand(0, round_rand(casualty_scale,
                                   (bstrength + 1.0) / (b2strength + 1.0))));
    report.defender_mil_casualties = std::min(
        s2.troops(),
        uniform_rand(0, round_rand(casualty_scale,
                                   (bstrength + 1.0) / (b2strength + 1.0))));
    s2.remove_popn(report.defender_civ_casualties, alien.mass);
    s2.remove_troops(report.defender_mil_casualties, alien.mass);

    report.defender_damage =
        std::min(100, uniform_rand(0, round_rand(25.0 * (bstrength + 1.0) /
                                                 (b2strength + 1.0))));
    if (s2.apply_damage(report.defender_damage).destroyed) {
      em.kill_ship(attacker_player, s2);
    }
    return;
  }

  s2.clear_crew(alien.mass);
  if (report.boobytrap_triggered) {
    report.booby_damage = static_cast<damage_t>(
        std::min<resource_t>(100, uniform_rand(0, 10 * s2.destruct())));
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
    alien.adjust_morale(race, race.fighters);
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

      report.attack_strength = report.surviving_boarders *
                               (what == PopulationType::MIL
                                    ? MILITARY_COMBAT_MULTIPLIER * race.fighters
                                    : 1) *
                               percent_to_fraction(race.tech) *
                               morale_factor(race.morale - alien.morale);
      report.defense_strength = (s2.popn() + MILITARY_COMBAT_MULTIPLIER *
                                                 s2.troops() * alien.fighters) *
                                percent_to_fraction(alien.tech) *
                                morale_factor(alien.morale - race.morale);

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
  if (boarders < 0 || (s2.max_crew() > 0 && boarders <= 0)) {
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
    em.mutate_universe([&](Universe& u) { u.deduct_AP(player, 1); });
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

namespace {

constexpr double calculate_boarding_attack_strength(population_t boarders,
                                                    PopulationType what,
                                                    const Race& attacker,
                                                    const Race& defender,
                                                    const Sector& sect) {
  const population_t force_weight =
      what == PopulationType::MIL
          ? attacker.fighters * MILITARY_COMBAT_MULTIPLIER
          : 1;
  return static_cast<double>(boarders * force_weight) *
         percent_to_fraction(attacker.tech) *
         (attacker.sector_compatibility(sect) + COMBAT_FLOOR_EPSILON) *
         sect.combat_defense_factor() *
         morale_factor(attacker.morale - defender.morale);
}

constexpr double calculate_boarding_defense_strength(const Ship& ship,
                                                     const Race& defender,
                                                     const Race& attacker) {
  const population_t crew_weight =
      ship.popn() +
      ship.troops() * MILITARY_COMBAT_MULTIPLIER * defender.fighters;
  const double hull_integrity_factor = percent_to_fraction(100 - ship.damage());
  return static_cast<double>(crew_weight) * percent_to_fraction(defender.tech) *
         (static_cast<double>(ship.effective_armor()) + COMBAT_FLOOR_EPSILON) *
         hull_integrity_factor *
         morale_factor(defender.morale - attacker.morale);
}

void resolve_manned_landed_boarding(EntityManager& em, Ship& ship,
                                    const Sector& sect, const Race& race,
                                    const Race& alien, population_t boarders,
                                    PopulationType what,
                                    player_t attacker_player,
                                    LandedBoardingResult& res) {
  const auto old_popn = ship.popn();
  const auto old_troops = ship.troops();
  res.had_defenders = true;
  res.attack_strength =
      calculate_boarding_attack_strength(boarders, what, race, alien, sect);
  res.defense_strength = calculate_boarding_defense_strength(ship, alien, race);

  const auto casualty_scale = std::min(boarders, old_popn + old_troops);
  if (res.attack_strength > 0.0) {
    res.attacker_casualties = std::min(
        boarders, uniform_rand(0, round_rand(casualty_scale,
                                             (res.defense_strength + 1.0) /
                                                 (res.attack_strength + 1.0))));
  }
  if (res.defense_strength > 0.0) {
    const double ratio =
        (res.attack_strength + 1.0) / (res.defense_strength + 1.0);
    res.defender_civ_casualties =
        std::min(old_popn, uniform_rand(0, round_rand(casualty_scale, ratio)));
    res.defender_mil_casualties = std::min(
        old_troops, uniform_rand(0, round_rand(casualty_scale, ratio)));
    res.ship_damage = static_cast<damage_t>(std::min(
        100,
        uniform_rand(0, round_rand(MAX_BOARDING_COLLATERAL_DAMAGE * ratio))));
    if (ship.apply_damage(res.ship_damage).destroyed) {
      em.kill_ship(attacker_player, ship);
    }
  }
  res.surviving_boarders = boarders - res.attacker_casualties;
  ship.remove_popn(res.defender_civ_casualties, alien.mass);
  ship.remove_troops(res.defender_mil_casualties, alien.mass);
}

void resolve_booby_trap_boarding(EntityManager& em, Ship& ship,
                                 population_t boarders,
                                 player_t attacker_player,
                                 LandedBoardingResult& res) {
  res.booby_damage = static_cast<damage_t>(
      std::min<resource_t>(100, uniform_rand(0, 10 * ship.destruct())));
  population_t casualties = 0;
  for (population_t i = 0; i < boarders; ++i) {
    casualties += (uniform_rand(1, 100) < res.booby_damage);
  }
  res.attacker_casualties = casualties;
  res.surviving_boarders = boarders - casualties;
  res.ship_damage = res.booby_damage;
  if (ship.apply_damage(res.booby_damage).destroyed) {
    em.kill_ship(attacker_player, ship);
  }
}

void finalize_landed_ship_capture(EntityManager& em, Ship& ship, Planet& planet,
                                  Sector& sect, const Race& race,
                                  PopulationType what, player_t attacker_player,
                                  governor_t attacker_governor,
                                  LandedBoardingResult& res) {
  population_t net_civ_lost = 0;
  population_t net_mil_lost = 0;
  const bool defenders_cleared = (ship.popn() + ship.troops() == 0);

  if (defenders_cleared && ship.alive() && res.surviving_boarders > 0) {
    res.captured = true;
    ship.owner() = attacker_player;
    ship.governor() = attacker_governor;
    if (what == PopulationType::CIV) {
      res.boarders_moved_in =
          std::min(res.surviving_boarders, ship.max_crew_capacity());
      ship.add_popn(res.boarders_moved_in, race.mass);
      net_civ_lost = res.boarders_moved_in + res.attacker_casualties;
    } else {
      res.boarders_moved_in =
          std::min(res.surviving_boarders, ship.available_mil());
      ship.add_troops(res.boarders_moved_in, race.mass);
      net_mil_lost = res.boarders_moved_in + res.attacker_casualties;
    }
    res.captured_ships = capture_stuff(em, ship);
    if (res.had_defenders && ship.type() != ShipType::OTYPE_FACTORY) {
      em.mutate_race(attacker_player, [&](Race& r) {
        em.mutate_race(res.old_defender_owner, [&](Race& a) {
          r.adjust_morale(a, static_cast<int>(ship.build_cost()));
        });
      });
    }
  } else if (what == PopulationType::CIV) {
    net_civ_lost = res.attacker_casualties;
  } else {
    net_mil_lost = res.attacker_casualties;
  }

  planet.adjust_sector_population(sect, attacker_player, -net_civ_lost,
                                  -net_mil_lost);
  res.sector_owner = sect.get_owner();
}

}  // namespace

LandedBoardingResult
execute_landed_ship_boarding(EntityManager& em, shipnum_t target_shipno,
                             population_t boarders, PopulationType what,
                             player_t attacker_player,
                             governor_t attacker_governor) {
  const auto& race = *em.peek_race(attacker_player);
  LandedBoardingResult res{
      .what = what,
      .attacker_player = attacker_player,
      .attacker_race_name = race.name,
      .surviving_boarders = boarders,
  };

  em.mutate_ship(target_shipno, [&](Ship& ship) {
    const auto old_owner = ship.owner();
    const auto& alien = *em.peek_race(old_owner);
    const auto& star = *em.peek_star(ship.storbits());

    res.old_defender_owner = old_owner;
    res.old_defender_gov = ship.governor();
    res.defender_race_name = alien.name;
    res.star_id = ship.storbits();
    res.star_name = star.get_name();
    res.planet_name = star.get_planet_name(ship.pnumorbits());
    res.sector_coords = ship.land_coords();
    res.allied_warning = race.is_allied_with(old_owner);
    res.defender_allied_with_attacker = alien.is_allied_with(attacker_player);
    res.defender_at_war_with_attacker = alien.is_at_war_with(attacker_player);

    em.mutate_planet_and_sectors(
        ship.storbits(), ship.pnumorbits(),
        [&](Planet& planet, SectorMap& smap) {
          auto& sect = smap.get(ship.land_coords());
          if (ship.popn() + ship.troops() > 0) {
            resolve_manned_landed_boarding(em, ship, sect, race, alien,
                                           boarders, what, attacker_player,
                                           res);
          } else if (ship.destruct() > 0) {
            resolve_booby_trap_boarding(em, ship, boarders, attacker_player,
                                        res);
          }
          finalize_landed_ship_capture(em, ship, planet, sect, race, what,
                                       attacker_player, attacker_governor, res);
        });

    res.location_display = dispshiploc(em, ship);
    res.ship_display = std::format("{}", ship);
    res.ship_total_damage = ship.damage();
    res.ship_alive = ship.alive();
    res.defender_has_remaining_crew = (ship.popn() + ship.troops() > 0);
  });

  return res;
}
