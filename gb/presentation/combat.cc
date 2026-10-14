// SPDX-License-Identifier: Apache-2.0

/// \file combat.cc
/// \brief Implementation of ship, planetary, boarding, and ground combat
/// presentation formatters.

module gb.presentation.combat;

import gb.presentation.core;
import gb.presentation.navigation;
import glaze.core;
import glaze.json;
import gb.repositories.glaze;
import strong_id;
import tabulate;

namespace GB::presentation {

std::string render_reactor_overload_event(const ReactorOverloadEvent& event) {
  switch (event.outcome) {
    case ReactorOverloadOutcome::ShipExploded:
      return std::format(
          "{}: Matter-antimatter EXPLOSION from overloaded crystal on {}\n",
          event.location_display, event.ship_display);
    case ReactorOverloadOutcome::CrystalDamaged:
      return std::format("{}: Crystal damaged from overloading on {}.\n",
                         event.location_display, event.ship_display);
  }
  std::unreachable();
}

std::string render_mech_attack_people_short(const MechAttackPeopleResult& res) {
  return std::format(
      "{}: {} {} {} [{}]\n", res.location_display, res.ship_display,
      (res.surviving_civ + res.surviving_mil) ? "attacked" : "slaughtered",
      res.defender_race_name, res.defender_player);
}

std::string render_mech_attack_people_long(const MechAttackPeopleResult& res) {
  return std::format("{}: {} {} {} [{}]\n"
                     "\tBattle at {} {}: {} guns fired on {} civ/{} mil\n"
                     "\tAttack: {:.3f}   Defense: {:.3f}.\n"
                     "\t{} civ/{} mil killed.\n",
                     res.location_display, res.ship_display,
                     (res.surviving_civ + res.surviving_mil) ? "attacked"
                                                             : "slaughtered",
                     res.defender_race_name, res.defender_player,
                     res.sector_coords, res.sector_condition, res.guns_fired,
                     res.initial_civ, res.initial_mil, res.attack_strength,
                     res.defense_strength, res.civ_killed, res.mil_killed);
}

std::string render_people_attack_mech_short(const PeopleAttackMechResult& res) {
  return std::format("{}: {} [{}] {} {}\n", res.location_display,
                     res.attacker_race_name, res.attacker_player,
                     res.mech_alive ? "attacked" : "DESTROYED",
                     res.ship_display);
}

std::string render_people_attack_mech_long(const PeopleAttackMechResult& res) {
  return std::format(
      "{}: {} [{}] {} {}\n"
      "\tBattle at {} {}: {} civ/{} mil assault {}\n"
      "\tAttack: {:.3f}   Defense: {:.3f}.\n"
      "\t{}% damage inflicted for a total of {}%\n"
      "\t{} civ/{} mil killed   {} prim/{} sec guns knocked out\n",
      res.location_display, res.attacker_race_name, res.attacker_player,
      res.mech_alive ? "attacked" : "DESTROYED", res.ship_display,
      res.target_coords, res.sector_condition, res.attacker_civ,
      res.attacker_mil, res.ship_type_name, res.attack_strength,
      res.defense_strength, res.damage_inflicted, res.total_damage,
      res.collateral.civilian_casualties, res.collateral.military_casualties,
      res.collateral.primary_guns_lost, res.collateral.secondary_guns_lost);
}

std::string render_ship_shot_short(const ShipShotResult& res) {
  if (res.attacker_kind == ShipShotAttackerKind::Planet) {
    return std::format("{} [{}] {} {}\n", res.target_location_display,
                       res.attacker_player.value,
                       res.target_alive ? "attacked" : "DESTROYED",
                       res.target_display);
  }
  return std::format(
      "{}: {} {} {}\n", res.target_location_display, res.attacker_display,
      res.target_alive ? "attacked" : "DESTROYED", res.target_display);
}

std::string render_ship_shot_long(const ShipShotResult& res) {
  std::string out = render_ship_shot_short(res);
  if (res.weapon == ShipShotWeaponKind::Radiation) {
    std::format_to(std::back_inserter(out),
                   "\tAttack: {} radiation\n"
                   "\t  Hits: {}\n"
                   "\t   Rad: {}% for a total of {}%\n",
                   res.strength, res.hits, res.radiation_dosage,
                   res.total_radiation);
    return out;
  }

  const std::string_view weapon_label = [weapon =
                                             res.weapon]() -> std::string_view {
    switch (weapon) {
      case ShipShotWeaponKind::Cew:
        return "strength CEW";
      case ShipShotWeaponKind::FocusedLaser:
        return "strength focused laser";
      case ShipShotWeaponKind::Laser:
        return "strength laser";
      case ShipShotWeaponKind::LightGuns:
      case ShipShotWeaponKind::Radiation:
        return "light guns";
      case ShipShotWeaponKind::MediumGuns:
        return "medium guns";
      case ShipShotWeaponKind::HeavyGuns:
        return "heavy guns";
    }
  }();

  std::format_to(std::back_inserter(out),
                 "\tAttack: {} {} at a range of {:.0f}\n"
                 "\t  Hits: {}  {}% probability\n",
                 res.strength, weapon_label, res.range, res.hits,
                 res.hit_probability);

  if (res.armor_reduced_to) {
    std::format_to(std::back_inserter(out), "\t\tArmor reduced to {}\n",
                   *res.armor_reduced_to);
  }
  if (res.penetrations > 0) {
    std::format_to(std::back_inserter(out),
                   "\t\t{} penetrations  eff armor={} defense={} prob={:.3f}\n",
                   res.penetrations, res.effective_armor, res.defense,
                   res.penetration_probability);
  }
  if (res.critical.count > 0) {
    const auto& sys = res.critical.systems;
    std::format_to(std::back_inserter(out),
                   "\t\t{} CRITICAL hits do {}% damage\n"
                   "\t\tSpecial systems damage: ",
                   res.critical.count, res.critical.damage);
    if (sys.cew_destroyed) out += "CEW ";
    if (sys.laser_destroyed) out += "Laser ";
    if (sys.cloak_destroyed) out += "Cloak ";
    if (sys.hyper_drive_destroyed) out += "Hyper-drive ";
    if (sys.reduced_max_speed) {
      std::format_to(std::back_inserter(out), "Speed={} ",
                     *sys.reduced_max_speed);
    }
    if (sys.reduced_armor) {
      std::format_to(std::back_inserter(out), "Armor={} ", *sys.reduced_armor);
    }
    out += '\n';
  }
  if (res.damage > 0) {
    std::format_to(std::back_inserter(out),
                   "\tDamage: {}% damage for a total of {}%\n", res.damage,
                   res.total_damage);
  }
  if (res.collateral.primary_guns_lost > 0 ||
      res.collateral.secondary_guns_lost > 0) {
    std::format_to(std::back_inserter(out),
                   "\t Other: {} primary/{} secondary guns destroyed\n",
                   res.collateral.primary_guns_lost,
                   res.collateral.secondary_guns_lost);
  }
  if (res.collateral.civilian_casualties > 0 ||
      res.collateral.military_casualties > 0) {
    std::format_to(
        std::back_inserter(out), "\tKilled: {} civ + {} mil casualties\n",
        res.collateral.civilian_casualties, res.collateral.military_casualties);
  }
  return out;
}

std::string render_bombard_short(const BombardResult& res) {
  return std::format("{} bombards {} [{}]\n", res.ship_display,
                     res.location_display, res.previous_sector_owner);
}

std::string render_bombard_long(const BombardResult& res) {
  return std::format("{} bombards {} [{}]\n\t{} sectors destroyed\n",
                     res.ship_display, res.location_display,
                     res.previous_sector_owner, res.sectors_destroyed);
}

std::string render_mine_detonation_notice(const MineDetonationReport& report) {
  return std::format("{} detonated at {}\n", report.ship_display,
                     report.orbit_display);
}

std::string
render_mine_planet_strike_telegram(const MineDetonationReport& report) {
  std::string out = std::format("{} detonated at {}\n", report.ship_display,
                                report.orbit_display);
  if (report.planet_strike && report.planet_strike->sectors_destroyed > 0) {
    std::format_to(std::back_inserter(out), " - {} sectors destroyed.",
                   report.planet_strike->sectors_destroyed);
  }
  out += '\n';
  return out;
}

std::string render_mine_detonation_report(const MineDetonationReport& report) {
  std::string out = report.planet_strike
                        ? render_mine_planet_strike_telegram(report)
                        : render_mine_detonation_notice(report);
  for (const auto& victim : report.ship_victims) {
    out += render_ship_shot_long(victim.shot);
  }
  return out;
}

std::string render_assault_error(const AssaultError& err) {
  switch (err.reason) {
    case AssaultErrorReason::ShipIrradiated:
      return std::format("{} is irradiated {}% and inactive.\n",
                         err.ship_display, err.radiation);
    case AssaultErrorReason::PodsCannotAssault:
      return "Sorry. Pods cannot be used to assault.\n";
    case AssaultErrorReason::ShipLandedOnCarrier:
      return "Your ship is landed on another ship.\n";
    case AssaultErrorReason::ShipAlreadyDocked:
      return "Your ship is already docked.\n";
    case AssaultErrorReason::NoCrew:
      return "You have no crew on this ship to assault with.\n";
    case AssaultErrorReason::NoTroops:
      return "You have no troops on this ship to assault with.\n";
    case AssaultErrorReason::CannotAssaultSelf:
      return "You can't dock with yourself!\n";
    case AssaultErrorReason::TargetNotFound:
      return "The ship wasn't found.\n";
    case AssaultErrorReason::NotInSameScope:
      return "Those ships are not in the same scope.\n";
    case AssaultErrorReason::CannotAssaultVonNeumann:
      return "You can't assault Von Neumann machines.\n";
    case AssaultErrorReason::TargetAlreadyLanded:
      return std::format("{} is already docked.\n", err.target_display);
    case AssaultErrorReason::TooFarAway:
      return std::format("{} must be {:.2f} or closer to {}.\n",
                         err.ship_display, err.max_distance,
                         err.target_display);
    case AssaultErrorReason::InsufficientFuel:
      return "Not enough fuel.\n";
    case AssaultErrorReason::IllegalBoarderCount:
      return std::format("Illegal number of boarders ({}).\n", err.boarders);
    case AssaultErrorReason::InsufficientUniverseAp:
      return "You need 1 universe action point.\n";
    case AssaultErrorReason::InsufficientStarAp:
      return "You don't have 1 action points there.\n";
    case AssaultErrorReason::UnmoorFailed:
      return "Failed to unmoor assaulted ship.\n";
  }
  std::unreachable();
}

std::string render_boarding_defender_telegram(const BoardingOutcomeReport& b) {
  std::string telegram =
      std::format("{} ASSAULTED by {} at {}\n"
                  "Your damage: {}%, theirs: {}%.\n",
                  b.target_display, b.attacker_display, b.target_orbit_display,
                  b.defender_damage, b.attacker_damage);
  if (b.boobytrap_triggered) {
    std::format_to(std::back_inserter(telegram),
                   "(Your boobytrap gave them {}% damage.)\n", b.booby_damage);
  }
  if (!b.attacker_alive) {
    telegram += "              Their ship DESTROYED!!!\n";
  }
  if (!b.defender_alive) {
    telegram += "              YOUR SHIP WAS DESTROYED!!!\n";
  }
  if (b.attacker_alive) {
    if (b.captured) {
      telegram += "CAPTURED!\n";
    } else if (b.defender_has_remaining_crew) {
      telegram += "You fought them off!\n";
    }
  } else {
    telegram += "The assault was too much for their ship..\n";
  }
  if (b.defender_alive) {
    if (!b.attacker_has_remaining_crew) {
      telegram += "You killed all their crew!\n";
    }
  } else {
    telegram += "Your ship was weakened too much!\n";
  }
  std::format_to(std::back_inserter(telegram),
                 "Casualties: Yours: {} mil/{} civ    Theirs: {} {}\n",
                 b.defender_mil_casualties, b.defender_civ_casualties,
                 b.attacker_casualties,
                 b.what == PopulationType::MIL ? "mil" : "civ");
  return telegram;
}

std::string render_boarding_news(const BoardingOutcomeReport& b) {
  return std::format("{} {} {} at {}.\n", b.attacker_display,
                     b.defender_alive ? (b.captured ? "CAPTURED" : "assaulted")
                                      : "DESTROYED",
                     b.target_display, b.attacker_orbit_display);
}

std::string render_ship_combat_exchange(const ShipCombatExchange& df) {
  std::string out;
  if (df.primary_overload_fizzled) {
    out += "No attack.\n";
  }
  if (df.primary_shot) {
    out += render_ship_shot_long(*df.primary_shot);
  }
  if (df.retaliation_shot) {
    out += render_ship_shot_long(*df.retaliation_shot);
  }
  for (const auto& escort : df.escort_shots) {
    if (escort.shot) {
      out += render_ship_shot_long(*escort.shot);
    }
  }
  return out;
}

std::string render_boarding_outcome(const BoardingOutcomeReport& b) {
  std::string out =
      std::format("Boarding strength :{:.2f}       Defense strength: {:.2f}.\n",
                  b.attack_strength, b.defense_strength);
  if (b.hyperdrive_deactivated) {
    out += "Hyper-drive deactivated.\n";
  }
  if (b.boobytrap_triggered) {
    std::format_to(std::back_inserter(out),
                   "Their boobytrap gave you {}% damage!)\n", b.booby_damage);
  }
  std::format_to(std::back_inserter(out), "Damage taken:  You: {}% (now {}%)\n",
                 b.attacker_damage, b.attacker_total_damage);
  if (!b.attacker_alive) {
    out += "              YOUR SHIP WAS DESTROYED!!!\n";
  }
  std::format_to(std::back_inserter(out), "              Them: {}% (now {}%)\n",
                 b.defender_damage, b.defender_total_damage);
  if (!b.defender_alive) {
    out += "              Their ship DESTROYED!!!  Boarders are dead.\n";
  }
  if (b.attacker_alive) {
    if (b.captured) {
      out += "VICTORY! the ship is yours!\n";
      if (b.surviving_boarders > 0) {
        std::format_to(std::back_inserter(out), "{} boarders move in.\n",
                       b.surviving_boarders);
      }
      out += render_captured_ships_report(b.captured_ships);
    } else if (b.defender_has_remaining_crew) {
      out += "The boarding was repulsed; try again.\n";
    }
  } else {
    out += "The assault was too much for your bucket of bolts.\n";
  }
  if (b.defender_alive) {
    if (b.target_max_crew > 0 && b.surviving_boarders == 0) {
      out += "Oh no! They killed your boarding party to the last man!\n";
    }
  } else {
    out += "The assault weakened their ship too much!\n";
  }
  std::format_to(std::back_inserter(out),
                 "Crew casualties: Yours: {} {}    Theirs: {} mil/{} civ\n",
                 b.attacker_casualties,
                 b.what == PopulationType::MIL ? "mil" : "civ",
                 b.defender_mil_casualties, b.defender_civ_casualties);
  return out;
}

std::string render_assault_result(const AssaultResult& res) {
  std::string out = std::format(
      "Distance to {}: {:.2f}.\n"
      "This maneuver will take {:.2f} fuel (of {:.2f}.)\n\n",
      res.target_display, res.distance, res.fuel_cost, res.initial_fuel);

  if (res.defensive_fire) {
    out += render_ship_combat_exchange(*res.defensive_fire);
  }
  if (res.boarding) {
    out += render_boarding_outcome(*res.boarding);
  }
  return out;
}

std::string render_fire_error_reason(const FireError& err) {
  switch (err.reason) {
    case FireErrorReason::ShipIrradiated:
      return std::format("{} is irradiated and inactive.\n", err.ship_display);
    case FireErrorReason::CannotFireAtSelf:
      return "Get real.\n";
    case FireErrorReason::TargetNotFound:
      return {};
    case FireErrorReason::AfvNotLanded:
      return std::format("{} isn't landed on a planet!\n", err.ship_display);
    case FireErrorReason::AfvTargetNotLanded:
      return std::format("{} isn't landed on a planet!\n", err.target_display);
    case FireErrorReason::LandedOnDifferentPlanets:
      return "Landed ships can only attack other landed ships if they are on "
             "the same planet!\n";
    case FireErrorReason::NotAdjacentOnPlanet:
      return "You are not adjacent to your target!\n";
    case FireErrorReason::NotEquippedForCew:
      return "That ship is not equipped to fire CEWs.\n";
    case FireErrorReason::NoCrystalMounted:
      return "You need to have a crystal mounted to fire CEWs.\n";
    case FireErrorReason::InsufficientCewFuel:
      return std::format("You need {} fuel to fire CEWs.\n", err.cew_strength);
    case FireErrorReason::CewLandedOriginOrTarget:
      return "CEWs cannot originate from or targeted to ships landed on "
             "planets.\n";
    case FireErrorReason::InsufficientUniverseAp:
      return "You need 1 universe action points.\n";
    case FireErrorReason::InsufficientStarAp:
      return "You don't have 1 action points there.\n";
    case FireErrorReason::NoAttackStrength:
      return "No attack.\n";
    case FireErrorReason::IllegalAttack:
      return "Illegal attack.\n";
  }
  return {};
}

std::string render_fire_error(const FireError& err) {
  std::string out;
  if (err.cew_strength > 0 &&
      err.reason != FireErrorReason::InsufficientCewFuel) {
    std::format_to(std::back_inserter(out), "CEW strength {}.\n",
                   err.cew_strength);
  }
  if (err.clamped_strength) {
    std::format_to(std::back_inserter(out), "{} set to {}\n",
                   err.clamped_is_laser ? "Laser strength" : "Guns",
                   *err.clamped_strength);
  }
  out += render_fire_error_reason(err);
  return out;
}

std::string render_fire_ship_result(const FireShipResult& res) {
  std::string out;
  if (res.cew_strength) {
    std::format_to(std::back_inserter(out), "CEW strength {}.\n",
                   *res.cew_strength);
  }
  if (res.clamped_strength) {
    std::format_to(std::back_inserter(out), "{} set to {}\n",
                   res.clamped_is_laser ? "Laser strength" : "Guns",
                   *res.clamped_strength);
  }
  out += render_ship_combat_exchange(res.exchange);
  return out;
}

std::string render_for_mode(UiMode mode, const ReactorOverloadEvent& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("reactor_overload", vm);
  }
  return render_reactor_overload_event(vm);
}

std::string render_for_mode(UiMode mode, const MechAttackPeopleResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("mech_attack_people", vm);
  }
  return render_mech_attack_people_long(vm);
}

std::string render_for_mode(UiMode mode, const PeopleAttackMechResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("people_attack_mech", vm);
  }
  return render_people_attack_mech_long(vm);
}

std::string render_for_mode(UiMode mode, const ShipShotResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("ship_shot", vm);
  }
  return render_ship_shot_long(vm);
}

std::string render_for_mode(UiMode mode, const BombardResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("bombard_result", vm);
  }
  return render_bombard_long(vm);
}

std::string render_for_mode(UiMode mode, const MineDetonationReport& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("mine_detonation", vm);
  }
  return render_mine_detonation_report(vm);
}

std::string render_for_mode(UiMode mode, const AssaultResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("assault", vm);
  }
  return render_assault_result(vm);
}

std::string render_for_mode(UiMode mode, const FireShipResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("fire_ship", vm);
  }
  return render_fire_ship_result(vm);
}

std::string render_tactical(const TacticalViewModel& vm) {
  std::string out;
  for (const auto& entry : vm.entries) {
    tabulate::Table header_table;
    header_table.format().hide_border().column_separator("  ");
    header_table.add_row({"#", "", "name", "tech", "guns", "armor", "size",
                          "dest", "fuel", "dam", "spd", "evad", "orbits"});
    header_table[0].format().font_style({tabulate::FontStyle::bold});

    if (entry.ship_header) {
      const auto& h = *entry.ship_header;
      const std::string name_str = h.active ? h.name : "INACTIVE";
      const std::string loc_suffix =
          h.landed_coords ? std::format(" ({})", *h.landed_coords) : "";
      const std::string inactive_suffix =
          h.active ? "" : std::format(" INACTIVE({})", h.radiation);

      header_table.add_row(
          {std::format("{}", h.number), std::format("{}", h.type_letter),
           name_str, std::format("{:.0f}", h.tech), h.battery_summary,
           std::format("{}", h.armor), std::format("{}", h.size),
           std::format("{}", h.destruct), std::format("{:.1f}", h.fuel),
           std::format("{}%", h.damage), std::format("{}", h.fspeed),
           h.evade ? "yes" : "",
           std::format("{}{}{}", h.orbits, loc_suffix, inactive_suffix)});
    } else if (entry.planet_header) {
      const auto& h = *entry.planet_header;
      header_table.add_row({"", "", std::format("(planet){}", h.planet_name),
                            std::format("{:.0f}", h.tech),
                            std::format("{}M", h.guns), "", "",
                            std::format("{}", h.destruct),
                            std::format("{}", h.fuel), "", "", "", ""});
    }

    std::format_to(std::back_inserter(out), "\n{}\n", header_table.str());

    if (entry.targets.empty()) {
      continue;
    }

    tabulate::Table tactical_table;
    tactical_table.format().hide_border().column_separator("  ");
    tactical_table.add_row({"Tactical: #", "own", "typ", "name", "rng", "(50%)",
                            "size", "spd", "evade", "hit", "dam", "loc"});
    tactical_table[0].format().font_style({tabulate::FontStyle::bold});

    for (const auto& target : entry.targets) {
      if (target.ship) {
        const auto& t = *target.ship;
        const std::string loc_str =
            t.landed_coords ? std::format("{}", *t.landed_coords) : "";
        const std::string status_suffix = t.active ? "" : " INACTIVE";

        tactical_table.add_row(
            {std::format("{}", t.number),
             std::format("{}{},{}", t.relation_indicator, t.owner, t.governor),
             std::format("{}", t.type_letter), std::format("{:.14}", t.name),
             std::format("{:.0f}", t.distance),
             std::format("{}", t.range_50_pct), std::format("{}", t.size),
             std::format("{}", t.speed), t.evade ? "yes" : "",
             std::format("{}%", t.hit_prob), std::format("{}%", t.damage),
             std::format("{}{}", loc_str, status_suffix)});
      } else if (target.planet) {
        const auto& t = *target.planet;
        tactical_table.add_row({"", "(planet)", "", t.planet_name,
                                std::format("{:.0f}", t.distance), "", "", "",
                                "", "", "", ""});
      }
    }

    std::format_to(std::back_inserter(out), "\n{}\n", tactical_table.str());
  }
  return out;
}

std::string render_json_tactical(const TacticalViewModel& vm) {
  return render_json_envelope("tactical", vm);
}

std::string render_for_mode(UiMode mode, const TacticalViewModel& vm) {
  return mode == UiMode::JSON ? render_json_tactical(vm) : render_tactical(vm);
}

std::string format_militia_error(const MilitiaError& err) {
  switch (err.reason) {
    case MilitiaErrorReason::PlanetEnslaved:
      return "That planet has been enslaved!\n";
    case MilitiaErrorReason::InvalidCoordinates:
      return "Illegal coordinates.\n";
    case MilitiaErrorReason::SectorNotOwned:
      return "You don't own that sector.\n";
    case MilitiaErrorReason::NoCiviliansToArm:
      return "You can't arm any civilians now.\n";
    case MilitiaErrorReason::InsufficientMoney:
      return std::format("You need {} money to enlist {} troops.\n",
                         err.required_money, err.requested_troops);
  }
  return "";
}

std::string render_arm_sector_result(const ArmSectorResult& vm) {
  return std::format(
      "{} population armed at a cost of {} (now {} civilians, {} military)\n"
      "This mobilization cost {} money.\n",
      vm.armed, vm.destruct_cost, vm.final_popn, vm.final_troops,
      vm.enlist_cost);
}

std::string render_json_arm_sector_result(const ArmSectorResult& vm) {
  return render_json_envelope("arm", vm);
}

std::string render_for_mode(UiMode mode, const ArmSectorResult& vm) {
  return mode == UiMode::JSON ? render_json_arm_sector_result(vm)
                              : render_arm_sector_result(vm);
}

std::string render_disarm_sector_result(const DisarmSectorResult& vm) {
  return std::format("{} troops disarmed (now {} civilians, {} military)\n",
                     vm.disarmed, vm.final_popn, vm.final_troops);
}

std::string render_json_disarm_sector_result(const DisarmSectorResult& vm) {
  return render_json_envelope("disarm", vm);
}

std::string render_for_mode(UiMode mode, const DisarmSectorResult& vm) {
  return mode == UiMode::JSON ? render_json_disarm_sector_result(vm)
                              : render_disarm_sector_result(vm);
}

}  // namespace GB::presentation
