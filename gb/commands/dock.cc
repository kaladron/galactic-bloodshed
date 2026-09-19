// SPDX-License-Identifier: Apache-2.0

/// \file dock.cc
/// \brief Dock a ship or assault target.

module;

import std;
import gb.entities;
import gb.services;
import notification;
import scnlib;
import session;

module commands;

namespace GB::commands {

namespace {

std::optional<PopulationType>
parse_assault_population_type(const command_t& argv, bool is_assault,
                              GameObj& g) {
  if (argv.size() < 3) {
    g.out << (is_assault ? "Assault what?\n" : "Dock with what?\n");
    return std::nullopt;
  }
  if (!is_assault || argv.size() < 5) {
    return PopulationType::MIL;
  }
  if (argv[4].starts_with("civ")) {
    return PopulationType::CIV;
  }
  if (argv[4].starts_with("mil")) {
    return PopulationType::MIL;
  }
  g.out << "Assault with what?\n";
  return std::nullopt;
}

bool validate_boarding_ship(const Ship& s, bool is_assault, PopulationType what,
                            GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  if (s.owner() != Playernum || !s.is_authorized_for(Governor) || !s.alive()) {
    return false;
  }
  if (!s.active()) {
    g.out << std::format("{} is irradiated {}% and inactive.\n", s, s.rad());
    return false;
  }
  if (is_assault && s.type() == ShipType::STYPE_POD) {
    g.out << "Sorry. Pods cannot be used to assault.\n";
    return false;
  }
  if (!is_assault) {
    if (s.docked() || s.whatorbits() == ScopeLevel::LEVEL_SHIP) {
      g.out << std::format("{} is already docked.\n", s);
      return false;
    }
    return true;
  }
  if (s.docked()) {
    g.out << "Your ship is already docked.\n";
    return false;
  }
  if (s.whatorbits() == ScopeLevel::LEVEL_SHIP) {
    g.out << "Your ship is landed on another ship.\n";
    return false;
  }
  if (what == PopulationType::CIV && !s.popn()) {
    g.out << "You have no crew on this ship to assault with.\n";
    return false;
  }
  if (what == PopulationType::MIL && !s.troops()) {
    g.out << "You have no troops on this ship to assault with.\n";
    return false;
  }
  return true;
}

struct DockTargetValidation {
  bool can_proceed{false};
  bool abort_loop{false};
  double dist{0.0};
  double fuel{0.0};
};

DockTargetValidation validate_target_ship_for_dock(const Ship& s,
                                                   shipnum_t ship2no,
                                                   bool is_assault,
                                                   GameObj& g) {
  DockTargetValidation val{};
  try {
    g.entity_manager.with_ship(ship2no, [&](const Ship& s2) {
      if (!is_assault && !g.check_commandable(s2)) {
        g.out << "You are not authorized to do this.\n";
        val.abort_loop = true;
        return;
      }
      if (s.whatorbits() != s2.whatorbits()) {
        g.out << "Those ships are not in the same scope.\n";
        return;
      }
      if (is_assault && s2.type() == ShipType::OTYPE_VN) {
        g.out << "You can't assault Von Neumann machines.\n";
        return;
      }
      bool invalid_dock_state =
          is_assault
              ? (s2.is_landed() || s2.whatorbits() == ScopeLevel::LEVEL_SHIP)
              : (s2.docked() || s2.whatorbits() == ScopeLevel::LEVEL_SHIP);
      if (invalid_dock_state) {
        g.out << std::format("{} is already docked.\n", s2);
        if (!is_assault) {
          val.abort_loop = true;
        }
        return;
      }

      val.dist = s2.coordinates().distance_to(s.coordinates());
      val.fuel = s.docking_fuel_cost(s2, is_assault);

      if (val.dist > DIST_TO_DOCK) {
        g.out << std::format("{} must be {:.2f} or closer to {}.\n", s,
                             DIST_TO_DOCK, s2);
        return;
      }
      if (val.fuel > s.fuel()) {
        g.out << "Not enough fuel.\n";
        return;
      }
      g.out << std::format("Distance to {}: {:.2f}.\n", s2, val.dist);
      g.out << std::format(
          "This maneuver will take {:.2f} fuel (of {:.2f}.)\n\n", val.fuel,
          s.fuel());
      val.can_proceed = true;
    });
  } catch (const EntityNotFoundError&) {
    g.out << "The ship wasn't found.\n";
    val.abort_loop = true;
  }
  return val;
}

bool deduct_assault_ap(const Ship& s, GameObj& g) {
  if (s.whatorbits() == ScopeLevel::LEVEL_UNIV) {
    if (!g.deduct_univ_ap(1)) {
      g.out << "You need 1 universe action point.\n";
      return false;
    }
    return true;
  }
  if (!g.deduct_ap(s.storbits(), 1)) {
    g.out << "You don't have 1 action points there.\n";
    return false;
  }
  return true;
}

void maneuver_ship_to_target(Ship& s, const Ship& s2, double fuel, GameObj& g) {
  use_fuel(s, fuel);
  s.set_coordinates(s2.coordinates() +
                    SystemCoordinates{static_cast<double>(int_rand(-1, 1)),
                                      static_cast<double>(int_rand(-1, 1))});
  if (s.hyper_drive().on) {
    s.hyper_drive().on = 0;
    g.out << "Hyper-drive deactivated.\n";
  }
}

void execute_peaceful_dock(Ship& s, Ship& s2, double fuel, GameObj& g) {
  maneuver_ship_to_target(s, s2, fuel, g);
  s.moor_together(s2);
  g.out << std::format("{} docked with {}.\n", s, s2);
}

struct BoardingCombatOutcome {
  population_t boarders{0};
  player_t old_defender_owner{0};
  governor_t old_defender_gov{0};
  int attacker_damage{0};
  int defender_damage{0};
  int booby_damage{0};
  population_t attacker_casualties{0};
  population_t defender_civ_casualties{0};
  population_t defender_mil_casualties{0};
  bool aborted{false};
};

void apply_boarding_casualties(Ship& s, Ship& s2, Race& alien, double bstrength,
                               double b2strength,
                               BoardingCombatOutcome& outcome, GameObj& g) {
  population_t casualty_scale =
      std::min(outcome.boarders, s2.troops() + s2.popn());

  if (b2strength > 0.0) {
    outcome.attacker_casualties =
        std::min(outcome.boarders,
                 static_cast<population_t>(int_rand(
                     0, round_rand(static_cast<double>(casualty_scale) *
                                   (b2strength + 1.0) / (bstrength + 1.0)))));
    outcome.boarders -= outcome.attacker_casualties;

    outcome.attacker_damage = std::min(
        100,
        int_rand(0, round_rand(25.0 * (b2strength + 1.0) / (bstrength + 1.0))));
    if (s.apply_damage(outcome.attacker_damage).destroyed) {
      g.entity_manager.kill_ship(g.player(), s);
    }

    outcome.defender_civ_casualties = std::min(
        s2.popn(), static_cast<population_t>(int_rand(
                       0, round_rand(static_cast<double>(casualty_scale) *
                                     (bstrength + 1.0) / (b2strength + 1.0)))));
    outcome.defender_mil_casualties =
        std::min(s2.troops(),
                 static_cast<population_t>(int_rand(
                     0, round_rand(static_cast<double>(casualty_scale) *
                                   (bstrength + 1.0) / (b2strength + 1.0)))));
    s2.remove_popn(outcome.defender_civ_casualties, alien.mass);
    s2.remove_troops(outcome.defender_mil_casualties, alien.mass);

    outcome.defender_damage = std::min(
        100,
        int_rand(0, round_rand(25.0 * (bstrength + 1.0) / (b2strength + 1.0))));
    if (s2.apply_damage(outcome.defender_damage).destroyed) {
      g.entity_manager.kill_ship(g.player(), s2);
    }
  } else {
    s2.clear_crew(alien.mass);
    if (!s2.max_crew() && s2.destruct()) {
      outcome.booby_damage = static_cast<int>(
          std::min<std::int64_t>(100, long_rand(0, 10 * s2.destruct())));
      outcome.attacker_damage += outcome.booby_damage;
      if (s.apply_damage(outcome.booby_damage).destroyed) {
        g.entity_manager.kill_ship(g.player(), s);
      }
    }
  }
}

void finalize_boarding_ownership_and_morale(Ship& s, Ship& s2, Race& race,
                                            Race& alien, PopulationType what,
                                            BoardingCombatOutcome& outcome,
                                            player_t Playernum) {
  if (!s2.popn() && !s2.troops() && s.alive() && s2.alive()) {
    s.moor_together(s2);
    s2.owner() = s.owner();
    s2.governor() = s.governor();
    if (what == PopulationType::MIL) {
      s2.add_troops(outcome.boarders, race.mass);
    } else {
      s2.add_popn(outcome.boarders, race.mass);
    }
    if (outcome.defender_civ_casualties + outcome.defender_mil_casualties > 0) {
      race.adjust_morale(alien, static_cast<int>(s2.build_cost()));
    }
  } else {
    if (what == PopulationType::MIL) {
      s.add_troops(outcome.boarders, race.mass);
    } else {
      s.add_popn(outcome.boarders, race.mass);
    }
    alien.adjust_morale(race, static_cast<int>(race.fighters));
  }

  alien.translate[Playernum] = std::min(alien.translate[Playernum] + 5, 100);
  race.translate[outcome.old_defender_owner] =
      std::min(race.translate[outcome.old_defender_owner] + 5, 100);

  if (!outcome.boarders && (s2.popn() + s2.troops())) {
    alien.translate[Playernum] = std::min(alien.translate[Playernum] + 25, 100);
  }
  if (s2.owner() == Playernum) {
    race.translate[outcome.old_defender_owner] =
        std::min(race.translate[outcome.old_defender_owner] + 25, 100);
  }
}

BoardingCombatOutcome resolve_boarding_combat(const command_t& argv, Ship& s,
                                              Ship& s2, PopulationType what,
                                              double fuel, GameObj& g) {
  BoardingCombatOutcome outcome{};
  player_t Playernum = g.player();

  g.entity_manager.mutate_race(Playernum, [&](Race& race) {
    g.entity_manager.mutate_race(s2.owner(), [&](Race& alien) {
      outcome.boarders = (what == PopulationType::CIV) ? s.popn() : s.troops();
      if (argv.size() >= 4) {
        if (auto scan_res = scn::scan<population_t>(argv[3], "{}")) {
          outcome.boarders = std::min(outcome.boarders, scan_res->value());
        }
      }
      if (outcome.boarders > s2.max_crew()) {
        outcome.boarders = s2.max_crew();
      }
      if (s2.max_crew() && outcome.boarders <= 0) {
        g.out << std::format("Illegal number of boarders ({}).\n",
                             outcome.boarders);
        outcome.aborted = true;
        return;
      }

      outcome.old_defender_owner = s2.owner();
      outcome.old_defender_gov = s2.governor();
      if (what == PopulationType::MIL) {
        s.remove_troops(outcome.boarders, race.mass);
      } else {
        s.remove_popn(outcome.boarders, race.mass);
      }

      double bstrength =
          outcome.boarders *
          (what == PopulationType::MIL ? 10 * race.fighters : 1) * .01 *
          race.tech *
          morale_factor(static_cast<double>(race.morale - alien.morale));
      double b2strength =
          (s2.popn() + 10 * s2.troops() * alien.fighters) * .01 * alien.tech *
          morale_factor(static_cast<double>(alien.morale - race.morale));
      g.out << std::format(
          "Boarding strength :{:.2f}       Defense strength: {:.2f}.\n",
          bstrength, b2strength);

      maneuver_ship_to_target(s, s2, fuel, g);

      if (s2.docked() && s2.whatorbits() != ScopeLevel::LEVEL_SHIP &&
          s2.whatdest() == ScopeLevel::LEVEL_SHIP) {
        if (auto res = g.entity_manager.unmoor_ships(s2.number()); !res) {
          g.out << "Failed to unmoor assaulted ship.\n";
          outcome.aborted = true;
          return;
        }
      }

      apply_boarding_casualties(s, s2, alien, bstrength, b2strength, outcome,
                                g);
      finalize_boarding_ownership_and_morale(s, s2, race, alien, what, outcome,
                                             Playernum);
    });
  });

  return outcome;
}

void report_boarding_outcome(const Ship& s, const Ship& s2, PopulationType what,
                             const BoardingCombatOutcome& outcome, GameObj& g) {
  player_t Playernum = g.player();
  governor_t Governor = g.governor();

  std::string telegram = std::format("{} ASSAULTED by {} at {}\n", s2, s,
                                     prin_ship_orbits(g.entity_manager, s2));
  telegram += std::format("Your damage: {}%, theirs: {}%.\n",
                          outcome.defender_damage, outcome.attacker_damage);
  if (!s2.max_crew() && s2.destruct()) {
    telegram += std::format("(Your boobytrap gave them {}% damage.)\n",
                            outcome.booby_damage);
    g.out << std::format("Their boobytrap gave you {}% damage!)\n",
                         outcome.booby_damage);
  }
  g.session_registry.notify_player(
      Playernum, Governor,
      std::format("Damage taken:  You: {}% (now {}%)\n",
                  outcome.attacker_damage, s.damage()));
  if (!s.alive()) {
    g.out << "              YOUR SHIP WAS DESTROYED!!!\n";
    telegram += "              Their ship DESTROYED!!!\n";
  }
  g.out << std::format("              Them: {}% (now {}%)\n",
                       outcome.defender_damage, s2.damage());
  if (!s2.alive()) {
    g.out << "              Their ship DESTROYED!!!  Boarders are dead.\n";
    telegram += "              YOUR SHIP WAS DESTROYED!!!\n";
  }
  if (s.alive()) {
    if (s2.owner() == Playernum) {
      telegram += "CAPTURED!\n";
      g.out << "VICTORY! the ship is yours!\n";
      if (outcome.boarders) {
        g.out << std::format("{} boarders move in.\n", outcome.boarders);
      }
      capture_stuff(s2, g);
    } else if (s2.popn() + s2.troops()) {
      g.out << "The boarding was repulsed; try again.\n";
      telegram += "You fought them off!\n";
    }
  } else {
    g.out << "The assault was too much for your bucket of bolts.\n";
    telegram += "The assault was too much for their ship..\n";
  }
  if (s2.alive()) {
    if (s2.max_crew() && !outcome.boarders) {
      g.out << "Oh no! They killed your boarding party to the last man!\n";
    }
    if (!s.popn() && !s.troops()) {
      telegram += "You killed all their crew!\n";
    }
  } else {
    g.out << "The assault weakened their ship too much!\n";
    telegram += "Your ship was weakened too much!\n";
  }
  telegram += std::format(
      "Casualties: Yours: {} mil/{} civ    Theirs: {} {}\n",
      outcome.defender_mil_casualties, outcome.defender_civ_casualties,
      outcome.attacker_casualties, what == PopulationType::MIL ? "mil" : "civ");
  g.out << std::format(
      "Crew casualties: Yours: {} {}    Theirs: {} mil/{} civ\n",
      outcome.attacker_casualties, what == PopulationType::MIL ? "mil" : "civ",
      outcome.defender_mil_casualties, outcome.defender_civ_casualties);
  warn_player(g.session_registry, g.entity_manager, outcome.old_defender_owner,
              outcome.old_defender_gov, telegram);

  auto news = std::format(
      "{} {} {} at {}.\n", s,
      s2.alive() ? (s2.owner() == Playernum ? "CAPTURED" : "assaulted")
                 : "DESTROYED",
      s2, prin_ship_orbits(g.entity_manager, s));
  if (s2.owner() == Playernum || !s2.alive()) {
    post(g.entity_manager, news, NewsType::COMBAT);
  }
  notify_star(g.session_registry, g.entity_manager, Playernum, Governor,
              s.storbits(), news);
}

bool process_single_ship_dock(const command_t& argv, Ship& s, bool is_assault,
                              PopulationType what, GameObj& g,
                              bool& should_abort_loop) {
  if (!ship_matches_filter(argv[1], s)) {
    return false;
  }
  if (!validate_boarding_ship(s, is_assault, what, g)) {
    return false;
  }

  auto shiptmp = string_to_shipnum(argv[2]);
  if (!shiptmp) {
    g.out << "Invalid ship number.\n";
    return false;
  }
  shipnum_t ship2no = *shiptmp;

  if (s.number() == ship2no) {
    g.out << "You can't dock with yourself!\n";
    return false;
  }

  auto target_val = validate_target_ship_for_dock(s, ship2no, is_assault, g);
  if (target_val.abort_loop) {
    should_abort_loop = true;
    return false;
  }
  if (!target_val.can_proceed) {
    return false;
  }

  if (is_assault && !deduct_assault_ap(s, g)) {
    return false;
  }

  if (is_assault) {
    command_t fire_argv{"fire-from-dock", std::format("{}", ship2no),
                        std::format("{}", s.number())};
    GB::commands::fire(fire_argv, g);
    if (!s.alive()) {
      return false;
    }
    bool s2_alive = true;
    g.entity_manager.with_ship(ship2no,
                               [&](const Ship& s2) { s2_alive = s2.alive(); });
    if (!s2_alive) {
      should_abort_loop = true;
      return false;
    }
  }

  bool completed = false;
  g.entity_manager.mutate_ship(ship2no, [&](Ship& s2) {
    if (is_assault) {
      auto outcome =
          resolve_boarding_combat(argv, s, s2, what, target_val.fuel, g);
      if (outcome.aborted) {
        return;
      }
      report_boarding_outcome(s, s2, what, outcome, g);
    } else {
      execute_peaceful_dock(s, s2, target_val.fuel, g);
    }
    s.notified() = s2.notified() = 0;
    completed = true;
  });

  return completed;
}

bool do_dock(const command_t& argv, GameObj& g, bool is_assault) {
  auto pop_type_opt = parse_assault_population_type(argv, is_assault, g);
  if (!pop_type_opt) {
    return false;
  }

  bool any_docked = false;
  ShipList ships(g);
  for (auto ship_handle : ships) {
    bool should_abort_loop = false;
    if (process_single_ship_dock(argv, *ship_handle, is_assault, *pop_type_opt,
                                 g, should_abort_loop)) {
      any_docked = true;
    }
    if (should_abort_loop) {
      return any_docked;
    }
  }

  return any_docked;
}

}  // namespace

bool dock(const command_t& argv, GameObj& g) {
  return do_dock(argv, g, false);
}

bool assault(const command_t& argv, GameObj& g) {
  return do_dock(argv, g, true);
}

const CommandDescriptor dock_cmd{
    .name = "dock",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 3,
    .syntax = "dock <ship> <target_ship>",
    .description = "Dock a ship with another ship",
    .handler = &dock,
};

const CommandDescriptor assault_cmd{
    .name = "assault",
    .roles = {.no_guests = true},
    .scopes = AllowedScopes::any(),
    .ap = APCost::dynamic(),
    .min_args = 3,
    .syntax = "assault <ship> <target_ship> [<boarders>] [civilians|military]",
    .description = "Assault and attempt to capture a target ship",
    .handler = &assault,
};

}  // namespace GB::commands
