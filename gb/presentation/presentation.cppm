// SPDX-License-Identifier: Apache-2.0

/// \file presentation.cppm
/// \brief Tier-6a Presentation and Protocol Rendering module (ASCII and Glaze
/// JSON).

export module gb.presentation;

import strong_id;
import glaze.core;
import glaze.json;
import gb.entities;
import gb.services;
import gb.mechanics;
import gb.repositories.glaze;
import tabulate;
import std;

export namespace GB::presentation {

/// Wire-protocol JSON envelope wrapping a typed command result, view model, or
/// asynchronous domain event without copying the payload.
template <typename T>
struct JsonEnvelope {
  std::string_view type{};
  const T* data{nullptr};
};

/// Serializes `data` into a newline-delimited JSON envelope:
/// `{"type":"<type>","data":<json>}\n`.
/// Uses a single named return variable (`out`) for guaranteed NRVO.
template <typename T>
[[nodiscard]] std::string render_json_envelope(std::string_view type,
                                               const T& data) {
  std::string out;
  const JsonEnvelope<T> envelope{.type = type, .data = &data};
  if (const auto ec = glz::write_json(envelope, out); !ec) {
    out.push_back('\n');
  }
  return out;
}

/// Formats a `TripEstimate` into its ASCII presentation string using a single
/// named return variable (`out`) for guaranteed NRVO.
[[nodiscard]] inline std::string render_trip_estimate(const TripEstimate& est) {
  std::string out;
  if (est.launch_gravity_fuel > 0.00) {
    std::format_to(
        std::back_inserter(out),
        "Total Distance = {:.2f}   Number of Segments = {}\nFuel = {:.2f} "
        "({:.2f} used to launch from {})\n  ",
        est.distance, est.segments, est.fuel_used, est.launch_gravity_fuel,
        est.launch_planet_name);
  } else {
    std::format_to(
        std::back_inserter(out),
        "Total Distance = {:.2f}   Number of Segments = {}\nFuel = {:.2f}   ",
        est.distance, est.segments, est.fuel_used);
  }

  switch (est.arrival_status) {
    case ArrivalTimeStatus::ServerStateUnavailable:
      out += "Server state unavailable.\n";
      break;
    case ArrivalTimeStatus::SegmentDiscrepancy:
      out += "Estimated arrival time not available due to segment # "
             "discrepancy.\n";
      break;
    case ArrivalTimeStatus::Available: {
      std::time_t arrival = est.estimated_arrival_time;
      std::format_to(std::back_inserter(out), "ESTIMATED Arrival Time: {}\n",
                     std::ctime(&arrival));
      break;
    }
  }
  return out;
}

/// Renders a `PlanetMapViewModel` as a 2D ASCII planetary map with X/Y
/// coordinate headers, ANSI SGR reverse-video (`\x1b[7m...\x1b[27m`) for
/// `inverse` cells, and a `tabulate::Table` for planetary statistics.
[[nodiscard]] inline std::string
render_ascii_planet_map(const PlanetMapViewModel& vm) {
  std::string out;
  std::format_to(std::back_inserter(out), "     {}\n", vm.planet_name);

  if (vm.dimensions.x >= 10) {
    out += "   ";
    for (const int x : std::views::iota(0, vm.dimensions.x)) {
      out.push_back(static_cast<char>('0' + ((x / 10) % 10)));
    }
    out.push_back('\n');
  }

  out += "   ";
  for (const int x : std::views::iota(0, vm.dimensions.x)) {
    out.push_back(static_cast<char>('0' + (x % 10)));
  }
  out.push_back('\n');

  if (vm.dimensions.x > 0 && vm.dimensions.y > 0) {
    bool in_inverse = false;
    for (const auto& cell : vm.sectors) {
      if (cell.coords.x == 0) {
        std::format_to(std::back_inserter(out), "{:02d} ", cell.coords.y);
      }
      if (cell.inverse && !in_inverse) {
        out += "\x1b[7m";
        in_inverse = true;
      } else if (!cell.inverse && in_inverse) {
        out += "\x1b[27m";
        in_inverse = false;
      }
      out.push_back(cell.glyph);
      if (cell.coords.x + 1 == vm.dimensions.x) {
        if (in_inverse) {
          out += "\x1b[27m";
          in_inverse = false;
        }
        out.push_back('\n');
      }
    }
  }
  out.push_back('\n');

  std::format_to(
      std::back_inserter(out),
      "Type: {:<8}   Sects {:<7}: {:<3}   Aliens:", vm.planet_type_name,
      vm.is_metamorph ? "covered" : "owned", vm.sectors_owned);
  if (vm.aliens_unknown) {
    out += "???";
  } else if (vm.aliens.empty()) {
    out += "(none)";
  } else {
    for (const auto& alien : vm.aliens) {
      std::format_to(std::back_inserter(out), "{}{}", alien.at_war ? '*' : ' ',
                     alien.player);
    }
  }
  out.push_back('\n');

  std::string compat_str = std::format("{:.2f}%", vm.compatibility);
  if (vm.toxicity > 50) {
    std::format_to(std::back_inserter(compat_str), " ({}% TOXIC)", vm.toxicity);
  }

  tabulate::Table stats_table;
  stats_table.format().hide_border().column_separator(" ");
  stats_table.column(0).format().width(20).font_align(
      tabulate::FontAlign::right);
  stats_table.column(1).format().width(10);
  stats_table.column(2).format().width(20).font_align(
      tabulate::FontAlign::right);
  stats_table.column(3).format().width(26);

  stats_table.add_row({"Guns :", std::format("{}", vm.guns),
                       "Mob Points :", std::format("{}", vm.mob_points)});
  stats_table.add_row(
      {"Mobilization :", std::format("{} ({})", vm.comread, vm.mob_set),
       "Compatibility :", compat_str});
  stats_table.add_row(
      {"Resource stockpile :", std::format("{}", vm.resource_stockpile),
       "Fuel stockpile :", std::format("{}", vm.fuel_stockpile)});
  stats_table.add_row(
      {"Destruct cap :", std::format("{}", vm.destruct_cap),
       vm.is_metamorph ? "Tons of biomass :" : "Total Population :",
       std::format("{} ({}/{})", vm.player_popn, vm.total_popn,
                   vm.effective_maxpopn)});
  stats_table.add_row(
      {"Crystals :", std::format("{}", vm.crystals), "Ground forces :",
       std::format("{} ({})", vm.player_troops, vm.total_troops)});

  out += stats_table.str();
  out.push_back('\n');

  std::format_to(std::back_inserter(out),
                 "{} Total Resource Deposits     Tax rate {}%  New {}%\n"
                 "Estimated Production Next Update : {:.2f}\n",
                 vm.total_resources, vm.tax, vm.newtax, vm.est_production);

  if (vm.slaved_to) {
    std::format_to(std::back_inserter(out), "      ENSLAVED to player {};\n",
                   *vm.slaved_to);
  }
  if (vm.primary_unstable) {
    out += "WARNING! This planet's primary is unstable.\n";
  }
  return out;
}

/// Serializes a `PlanetMapViewModel` into a newline-delimited Glaze JSON
/// envelope (`{"type":"map","data":{...}}\n`).
[[nodiscard]] inline std::string
render_json_planet_map(const PlanetMapViewModel& vm) {
  return render_json_envelope("map", vm);
}

/// Formats a `PlanetBuildError` into its user-facing error diagnostic string.
[[nodiscard]] inline std::string
format_planet_build_error(const PlanetBuildError& err) {
  std::string out;
  switch (err.reason) {
    case PlanetBuildErrorReason::EnslavedByForeignPlayer:
      std::format_to(std::back_inserter(out),
                     "This planet is enslaved by player {}.\n",
                     err.enslaving_player);
      break;
    case PlanetBuildErrorReason::NotAuthorizedInSystem:
      out = "You are not authorized in this system.\n";
      break;
  }
  return out;
}

/// Formats a `ShipBuildError` into its user-facing error diagnostic string.
[[nodiscard]] inline std::string format_ship_build_error(ShipBuildError err) {
  switch (err) {
    case ShipBuildError::ShipDead:
      return "Has been destroyed.\n";
    case ShipBuildError::ShipIrradiated:
      return "This ship is irradiated and inactive.\n";
    case ShipBuildError::NotOwner:
      return "You do not own this ship.\n";
    case ShipBuildError::NotAuthorizedGovernor:
      return "You are not authorized to do this.\n";
    case ShipBuildError::CannotConstructShips:
      return "This ship cannot construct other ships.\n";
    case ShipBuildError::NoCrew:
      return "This ship has no crew.\n";
    case ShipBuildError::ShipDocked:
      return "Undock this ship first.\n";
    case ShipBuildError::ShipDamaged:
      return "This ship is damaged and cannot build.\n";
    case ShipBuildError::FactoryNotOnline:
      return "This factory is not online.\n";
    case ShipBuildError::FactoryNotLanded:
      return "Factories must be landed on a planet.\n";
  }
  std::unreachable();
}

/// Formats a `CreatedShipSummary` into its ASCII presentation string.
[[nodiscard]] inline std::string
render_created_ship_summary(const CreatedShipSummary& summary) {
  std::string out;
  if (summary.previous_toxicity && summary.updated_toxicity) {
    std::format_to(std::back_inserter(out),
                   "Toxin concentration on planet was {}%, now {}%.\n",
                   *summary.previous_toxicity, *summary.updated_toxicity);
  }
  std::format_to(std::back_inserter(out),
                 "{} built at a cost of {} resources.\nTechnology {:.1f}.\n",
                 summary.ship_display, summary.build_cost, summary.tech);
  if (summary.landed_sector) {
    std::format_to(std::back_inserter(out), "{} is on sector {}.\n",
                   summary.ship_display, *summary.landed_sector);
  }
  return out;
}

/// Formats an `InitializedShipReport` into its ASCII presentation string.
[[nodiscard]] inline std::string
render_initialized_ship_report(const InitializedShipReport& report) {
  std::string out;
  switch (report.ship_type) {
    case ShipType::STYPE_MINE:
      out += "Mine disarmed.\nTrigger radius set at 100.\n";
      break;
    case ShipType::OTYPE_TRANSDEV:
      out += "Receive OFF.  Change with order.\n";
      break;
    case ShipType::OTYPE_AP:
      out += "Processor OFF.\n";
      break;
    case ShipType::OTYPE_STELE:
    case ShipType::OTYPE_GTELE:
      std::format_to(std::back_inserter(out), "Telescope range is {:.2f}.\n",
                     report.tele_range);
      break;
    default:
      break;
  }
  if (report.damage) {
    std::format_to(
        std::back_inserter(out),
        "Warning: This ship is constructed with a {}% damage level.\n",
        report.damage);
    if (!report.can_repair && report.has_crew_capacity) {
      out += "It will need resources to become fully operational.\n";
    }
  }
  if (report.can_repair && report.has_crew_capacity) {
    out += "This ship does not need resources to repair.\n";
  }
  if (report.ship_type == ShipType::OTYPE_FACTORY) {
    out += "This factory may not begin repairs until it has been activated.\n";
  }
  if (!report.has_crew_capacity) {
    out += "This ship is robotic, and may not repair itself.\n";
  }

  std::format_to(std::back_inserter(out),
                 "Loaded with {} crew and {:.1f} fuel.\n", report.loaded_crew,
                 report.loaded_fuel);
  return out;
}

/// Formats a `CapturedShipsReport` into its ASCII presentation string.
[[nodiscard]] inline std::string
render_captured_ships_report(const CapturedShipsReport& report) {
  std::string out;
  for (const auto& ship : report.captured_ships) {
    std::format_to(std::back_inserter(out), "{} CAPTURED!\n",
                   ship.ship_display);
  }
  return out;
}

/// Formats an `OrderError` into its user-facing error diagnostic string.
[[nodiscard]] inline std::string format_order_error(const OrderError& err) {
  switch (err.reason) {
    case OrderErrorReason::ShipIrradiated:
      return std::format("{} is irradiated ({}); it cannot be given orders.\n",
                         err.ship_display, err.radiation);
    case OrderErrorReason::ShipHasNoCrew:
      return std::format("{} has no crew and is not a robotic ship.\n",
                         err.ship_display);
    case OrderErrorReason::CannotBeAssignedOrders:
      return "That ship cannot be assigned those orders.\n";
    case OrderErrorReason::ShipDockedUseLaunchOrUndock:
      return "That ship is docked. Use 'launch' or 'undock' first.\n";
    case OrderErrorReason::NoHyperDriveCapability:
      return "This ship does not have hyper drive capability.\n";
    case OrderErrorReason::DestinationMustBeStarOrPlanet:
      return "Destination must be star or planet.\n";
    case OrderErrorReason::CannotProtectSelf:
      return "You can't do that.\n";
    case OrderErrorReason::CannotProtect:
      return "That ship cannot protect.\n";
    case OrderErrorReason::CannotBeLaunched:
      return "That ship cannot be launched.\n";
    case OrderErrorReason::ShipDockedUndockOrLaunchFirst:
      return "That ship is docked; use undock or launch first.\n";
    case OrderErrorReason::InvalidPlace:
      return err.place_error ? format_place_error(*err.place_error)
                             : std::string{};
    case OrderErrorReason::TargetShipOutOfRange:
      return "Warning: that ship is out of range.\n";
    case OrderErrorReason::SystemUnexplored:
      return "You haven't explored this system.\n";
    case OrderErrorReason::ShipTypeCannotRetaliate:
      return "This type of ship cannot be set to retaliate.\n";
    case OrderErrorReason::ShipCannotRetaliate:
      return "This ship cannot be set to retaliate.\n";
    case OrderErrorReason::NoLaser:
      return "No laser.\n";
    case OrderErrorReason::NotEquippedWithCombatLasers:
      return "This ship is not equipped with combat lasers.\n";
    case OrderErrorReason::NoCrystalMounted:
      return "You do not have a crystal mounted.\n";
    case OrderErrorReason::BadRouteNumber:
      return "Bad route number.\n";
    case OrderErrorReason::NoSpeedRating:
      return "This ship does not have a speed rating.\n";
    case OrderErrorReason::InvalidSpeed:
      return "Specify a positive speed.\n";
    case OrderErrorReason::InvalidSalvoGunCount:
      return "Specify a positive number of guns.\n";
    case OrderErrorReason::NoPrimaryGuns:
      return "This ship does not have primary guns.\n";
    case OrderErrorReason::NoSecondaryGuns:
      return "This ship does not have secondary guns.\n";
    case OrderErrorReason::InvalidBatteryGunCount:
      return "Specify a nonnegative number of guns.\n";
    case OrderErrorReason::OnlyMissilesCanScatter:
      return "Only missiles can be given this order.\n";
    case OrderErrorReason::OnlyMissilesCanBeDesignated:
      return "Only missiles can be designated for this.\n";
    case OrderErrorReason::InvalidDesignateCoords:
      return "Usage: order <ship> designate <x>,<y>\n";
    case OrderErrorReason::UseOnForFactory:
      return "Use \"on\" to bring factory online.\n";
    case OrderErrorReason::NoSwitchSetting:
      return "That ship does not have an on/off setting.\n";
    case OrderErrorReason::ShipBeingTransported:
      return "That ship is being transported.\n";
    case OrderErrorReason::NotTerraformerOrPlow:
      return "That ship is not a terraformer or a space plow.\n";
    case OrderErrorReason::EmptyCyclingMoveOrders:
      return "Cycling move orders can not be empty!\n";
    case OrderErrorReason::InvalidMoveDirection:
      return std::format("'{}' is not a valid move direction.\n",
                         err.invalid_move_char);
    case OrderErrorReason::CannotAssignTriggerRadius:
      return "This ship cannot be assigned a trigger radius.\n";
    case OrderErrorReason::NotATransporter:
      return "This ship is not a transporter.\n";
    case OrderErrorReason::CannotTransportToSelf:
      return "A transporter cannot transport to itself.\n";
    case OrderErrorReason::CannotAimShip:
      return "You can't aim that kind of ship.\n";
    case OrderErrorReason::NotEnoughManeuveringFuel:
      return std::format("Not enough maneuvering fuel ({:.2f}).\n",
                         err.required_fuel);
    case OrderErrorReason::MirrorDocked:
      return "docked; use undock or launch first.\n";
    case OrderErrorReason::AimDestinationError:
      return "Error in destination.\n";
    case OrderErrorReason::AimPlaceError:
      return std::format("{}Error in destination.\n",
                         err.place_error ? format_place_error(*err.place_error)
                                         : std::string{});
    case OrderErrorReason::ThisShipHasNoSwitch:
      return "This ship does not have an on/off setting.\n";
    case OrderErrorReason::DamagedShipsCannotBeActivated:
      return "Damaged ships cannot be activated.\n";
    case OrderErrorReason::ShipAlreadyActivated:
      return "This ship is already activated.\n";
    case OrderErrorReason::FactoryBeingTransported:
      return "The factory is currently being transported.\n";
    case OrderErrorReason::InsufficientHabitatResourcesForFactory:
      return std::format(
          "You don't have {} resources on Habitat #{} to activate this "
          "factory.\n",
          err.required_resources, err.habitat_ship);
    case OrderErrorReason::InsufficientHabitatHangarForFactory:
      return std::format(
          "Not enough hanger space free on Habitat #{}. Need {} more.\n",
          err.habitat_ship, err.hangar_needed);
    case OrderErrorReason::CannotActivateFactoryHere:
      return "You cannot activate the factory here.\n";
    case OrderErrorReason::InsufficientPlanetResourcesForFactory:
      return std::format(
          "You don't have {} resources on the planet to activate this "
          "factory.\n",
          err.required_resources);
    case OrderErrorReason::CannotDeactivateFactory:
      return "You can't deactivate a factory once it's online. Consider "
             "using 'scrap'.\n";
  }
  std::unreachable();
}

/// Formats an `OrderUpdate` notice into its ASCII presentation string.
[[nodiscard]] inline std::string
render_order_update(const OrderUpdate& update) {
  std::string out;
  switch (update.notice) {
    case OrderUpdateNotice::None:
      break;
    case OrderUpdateNotice::MineArmed:
      out += "Mine armed and ready.\n";
      break;
    case OrderUpdateNotice::MineDisarmed:
      out += "Mine disarmed.\n";
      break;
    case OrderUpdateNotice::TransporterReady:
      out += "Transporter ready to receive.\n";
      break;
    case OrderUpdateNotice::TransporterStopped:
      out += "No longer receiving.\n";
      break;
    case OrderUpdateNotice::MoveTruncatedLength:
      std::format_to(std::back_inserter(out),
                     "Warning: that is more than {} moves.\n"
                     "These move orders have been truncated.\n",
                     update.max_moves);
      break;
    case OrderUpdateNotice::MoveTruncatedAfterModeChar:
      std::format_to(
          std::back_inserter(out),
          "Warning: '{}' should be the last character in the move order.\n"
          "These move orders have been truncated.\n",
          update.truncated_after_char);
      break;
    case OrderUpdateNotice::TransportTargetSet:
      std::format_to(std::back_inserter(out), "Target ship is {}.\n",
                     update.target_ship);
      break;
    case OrderUpdateNotice::Aimed:
      switch (update.survey_outcome) {
        case TelescopeSurveyOutcome::None:
          break;
        case TelescopeSurveyOutcome::NothingAtUniv:
          out += "There is nothing out here to aim at.\n";
          break;
        case TelescopeSurveyOutcome::NothingOfUseAtShip:
          out += "You can't see anything of use there.\n";
          break;
        case TelescopeSurveyOutcome::StarSurveyed:
          std::format_to(std::back_inserter(out),
                         "Star {}\nSurveyed, distance {}.\n", update.aim_target,
                         update.survey_distance);
          break;
        case TelescopeSurveyOutcome::StarTooFar:
          std::format_to(std::back_inserter(out),
                         "Star {}\nToo far to see ({}, max {}).\n",
                         update.aim_target, update.survey_distance,
                         update.tele_range);
          break;
        case TelescopeSurveyOutcome::PlanetSurveyed:
          std::format_to(std::back_inserter(out),
                         "Planet {}\nSurveyed, distance {}.\n",
                         update.aim_target, update.survey_distance);
          break;
        case TelescopeSurveyOutcome::PlanetTooFar:
          std::format_to(std::back_inserter(out),
                         "Planet {}\nToo far to see ({}, max {}).\n",
                         update.aim_target, update.survey_distance,
                         update.tele_range);
          break;
      }
      std::format_to(std::back_inserter(out), "Aimed at {}\n",
                     update.aim_target);
      break;
    case OrderUpdateNotice::FactoryActivated:
      std::format_to(std::back_inserter(out),
                     "Factory activated at a cost of {} resources.\n",
                     update.factory_activation_cost);
      break;
  }
  return out;
}

/// Returns the ASCII table header for standing ship orders.
[[nodiscard]] inline std::string
render_ship_orders_header(const ShipOrdersHeader&) {
  return "    #       name       sp orbits     destin     options\n";
}

/// Formats a `ShipOrderStatus` into its ASCII presentation string.
[[nodiscard]] inline std::string
render_ship_order_status(const ShipOrderStatus& status) {
  std::string out = std::format(
      "{:5} {} {:14.14} {}{} {:10.10} {}{}{}{}\n", status.ship_number,
      status.type_letter, status.name, status.hyper_indicator, status.speed,
      status.orbits_display, status.destination_display, status.combat_options,
      status.navigation_options, status.specialty_options);

  if (status.has_hyperdrive_jump) {
    std::format_to(std::back_inserter(out),
                   "  *** distance {:.0f} - jump will cost {:.1f}f ***\n",
                   status.jump_distance, status.jump_fuel_cost);
    if (status.insufficient_fuel_capacity) {
      out += "Your ship cannot carry enough fuel to do this jump.\n";
    }
  }
  return out;
}

/// Formats a `ReactorOverloadEvent` into its ASCII combat report string.
[[nodiscard]] inline std::string
render_reactor_overload_event(const ReactorOverloadEvent& event) {
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

/// Formats a `MechAttackPeopleResult` into its short headline string (used for
/// star notifications and combat news).
[[nodiscard]] inline std::string
render_mech_attack_people_short(const MechAttackPeopleResult& res) {
  return std::format(
      "{}: {} {} {} [{}]\n", res.location_display, res.ship_display,
      (res.surviving_civ + res.surviving_mil) ? "attacked" : "slaughtered",
      res.defender_race_name, res.defender_player);
}

/// Formats a `MechAttackPeopleResult` into its full multi-line battle report.
[[nodiscard]] inline std::string
render_mech_attack_people_long(const MechAttackPeopleResult& res) {
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

/// Formats a `PeopleAttackMechResult` into its short headline string (used for
/// star notifications and combat news).
[[nodiscard]] inline std::string
render_people_attack_mech_short(const PeopleAttackMechResult& res) {
  return std::format("{}: {} [{}] {} {}\n", res.location_display,
                     res.attacker_race_name, res.attacker_player,
                     res.mech_alive ? "attacked" : "DESTROYED",
                     res.ship_display);
}

/// Formats a `PeopleAttackMechResult` into its full multi-line battle report.
[[nodiscard]] inline std::string
render_people_attack_mech_long(const PeopleAttackMechResult& res) {
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

/// Formats the short one-line summary for `ShipShotResult`
/// (`shoot_ship_to_ship` and `shoot_planet_to_ship`).
[[nodiscard]] inline std::string
render_ship_shot_short(const ShipShotResult& res) {
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

/// Formats the full multi-line combat report for `ShipShotResult`.
[[nodiscard]] inline std::string
render_ship_shot_long(const ShipShotResult& res) {
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

/// Formats the short one-line summary for `BombardResult`
/// (`shoot_ship_to_planet`).
[[nodiscard]] inline std::string
render_bombard_short(const BombardResult& res) {
  return std::format("{} bombards {} [{}]\n", res.ship_display,
                     res.location_display, res.previous_sector_owner);
}

/// Formats the full multi-line report for `BombardResult`
/// (`shoot_ship_to_planet`).
[[nodiscard]] inline std::string render_bombard_long(const BombardResult& res) {
  return std::format("{} bombards {} [{}]\n\t{} sectors destroyed\n",
                     res.ship_display, res.location_display,
                     res.previous_sector_owner, res.sectors_destroyed);
}

/// Formats the combat news/star notification line for `MineDetonationReport`.
[[nodiscard]] inline std::string
render_mine_detonation_notice(const MineDetonationReport& report) {
  return std::format("{} detonated at {}\n", report.ship_display,
                     report.orbit_display);
}

/// Formats the planetary strike telegram for `MineDetonationReport`.
[[nodiscard]] inline std::string
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

/// Formats the full interactive command report for `MineDetonationReport`.
[[nodiscard]] inline std::string
render_mine_detonation_report(const MineDetonationReport& report) {
  std::string out = report.planet_strike
                        ? render_mine_planet_strike_telegram(report)
                        : render_mine_detonation_notice(report);
  for (const auto& victim : report.ship_victims) {
    out += render_ship_shot_long(victim.shot);
  }
  return out;
}

/// Formats a `DetonateError` into its ASCII error string.
[[nodiscard]] inline std::string render_detonate_error(DetonateError err) {
  switch (err) {
    case DetonateError::NotAMine:
      return "That is not a mine.\n";
    case DetonateError::NotActivated:
      return "The mine is not activated.\n";
    case DetonateError::DockedOrLanded:
      return "The mine is docked or landed.\n";
    case DetonateError::DetonationFailed:
      return "";
  }
  std::unreachable();
}

/// Formats a `JettisonError` into its ASCII error string.
[[nodiscard]] inline std::string
render_jettison_error(const JettisonError& err) {
  switch (err.reason) {
    case JettisonErrorReason::ShipLanded:
      return "Ship is landed, cannot jettison.\n";
    case JettisonErrorReason::ShipIrradiated:
      return std::format("{} is irradiated and inactive.\n", err.ship_display);
    case JettisonErrorReason::InvalidCommodity:
      return "No such commodity valid.\n";
    case JettisonErrorReason::NegativeAmount:
      return "Nice try.\n";
    case JettisonErrorReason::ExceedsAvailable:
      return std::format("You can jettison at most {}\n", err.max_available);
    case JettisonErrorReason::NothingToJettison:
      return "";
  }
  std::unreachable();
}

/// Formats a `JettisonResult` into its ASCII presentation string.
[[nodiscard]] inline std::string
render_jettison_result(const JettisonResult& res) {
  switch (res.commodity) {
    case JettisonCommodity::Crystals:
      return std::format("{} crystal{} jettisoned.\n", res.amount,
                         (res.amount == 1) ? "" : "s");
    case JettisonCommodity::Crew:
      return std::format("{} crew {} into deep space.\n"
                         "Complement of {} is now {}.\n",
                         res.amount,
                         (res.amount == 1) ? "hurls itself" : "hurl themselves",
                         res.ship_display, res.remaining_complement);
    case JettisonCommodity::Military:
      return std::format("{} military {} into deep space.\n"
                         "Complement of ship #{} is now {}.\n",
                         res.amount,
                         (res.amount == 1) ? "hurls itself" : "hurl themselves",
                         res.ship_number, res.remaining_complement);
    case JettisonCommodity::Destruct: {
      std::string out = std::format("{} destruct jettisoned.\n", res.amount);
      if (res.check_boobytrap) {
        std::format_to(std::back_inserter(out), "\n{} {}\n", res.ship_display,
                       res.still_boobytrapped ? "still boobytrapped."
                                              : "no longer boobytrapped.");
      }
      return out;
    }
    case JettisonCommodity::Fuel:
      return std::format("{} fuel jettisoned.\n", res.amount);
    case JettisonCommodity::Resources:
      return std::format("{} resources jettisoned.\n", res.amount);
  }
  std::unreachable();
}

/// Formats a `MountCrystalError` into its ASCII error string.
[[nodiscard]] inline std::string
render_mount_crystal_error(MountCrystalError err) {
  switch (err) {
    case MountCrystalError::NoCrystalMount:
      return "This ship is not equipped with a crystal mount.\n";
    case MountCrystalError::AlreadyMounted:
      return "You already have a crystal mounted.\n";
    case MountCrystalError::NoCrystalsOnBoard:
      return "You have no crystals on board.\n";
  }
  std::unreachable();
}

/// Formats a `MountCrystalResult` into its ASCII presentation string.
[[nodiscard]] inline std::string
render_mount_crystal_result(const MountCrystalResult&) {
  return "Mounted.\n";
}

/// Formats a `DismountCrystalError` into its ASCII error string.
[[nodiscard]] inline std::string
render_dismount_crystal_error(DismountCrystalError err) {
  switch (err) {
    case DismountCrystalError::NoCrystalMount:
      return "This ship is not equipped with a crystal mount.\n";
    case DismountCrystalError::NotMounted:
      return "You don't have a crystal mounted.\n";
    case DismountCrystalError::MaxCrystalsOnBoard:
      return "You can't dismount the crystal. Max allowed already on board.\n";
  }
  std::unreachable();
}

/// Formats a `DismountCrystalResult` into its ASCII presentation string.
[[nodiscard]] inline std::string
render_dismount_crystal_result(const DismountCrystalResult& res) {
  std::string out = "Dismounted.\n";
  if (res.hyperdrive_discharged) {
    out += "Discharged.\n";
  }
  if (res.laser_deactivated) {
    out += "Laser deactivated.\n";
  }
  return out;
}

/// Formats a `GrantShipResult` into its caller ASCII presentation string.
[[nodiscard]] inline std::string
render_grant_ship_result(const GrantShipResult& res) {
  return std::format("{} granted to \"{}\"\n", res.ship_display,
                     res.recipient_governor_name);
}

/// Formats a `GrantShipResult` into the recipient governor's notification.
[[nodiscard]] inline std::string
render_grant_ship_notification(const GrantShipResult& res) {
  return std::format("\"{}\" granted you {} at {}\n", res.donor_governor_name,
                     res.ship_display, res.orbits_display);
}

/// Formats a `ScrapError` into its ASCII error string.
[[nodiscard]] inline std::string render_scrap_error(ScrapError err) {
  switch (err) {
    case ScrapError::NoCrew:
      return "Can't scrap that ship - no crew.\n";
    case ScrapError::StarNotFound:
      return "Star not found.\n";
    case ScrapError::InsufficientUniverseAp:
      return "You need 1 universe action point.\n";
    case ScrapError::InsufficientStarAp:
      return "You don't have 1 action points there.\n";
    case ScrapError::OtherShipNotDocked:
      return "Warning, other ship not docked..\n";
  }
  std::unreachable();
}

/// Formats a `ScrapShipResult` into its ASCII presentation string.
[[nodiscard]] inline std::string
render_scrap_ship_result(const ScrapShipResult& res) {
  std::string out;
  if (res.toxin_released.has_value()) {
    std::format_to(std::back_inserter(out),
                   "WARNING: This will release {} toxin points back into the "
                   "atmosphere!!\n",
                   *res.toxin_released);
  }
  if (!res.reclaimed) {
    std::format_to(
        std::back_inserter(out),
        "{} is not landed or docked.\nNo resources can be reclaimed.\n",
        res.ship_display);
  } else {
    std::format_to(std::back_inserter(out), "{}: original cost: {}\n",
                   res.ship_display, res.original_cost);
    std::format_to(std::back_inserter(out),
                   "         scrap value{}: {} rp's.\n",
                   res.has_resource_stockpile ? "(with stockpile) " : "",
                   res.initial_scrap_value);
    if (res.foreign_sector_blocks_crew) {
      out += "You don't own this sector; no crew can be recovered.\n";
    }
    if (res.foreign_sector_blocks_crystals) {
      out += "You don't own this sector; no crystals can be recovered.\n";
    }
    if (res.resource_room_limit.has_value()) {
      std::format_to(std::back_inserter(out),
                     "(There is only room for {} resources.)\n",
                     *res.resource_room_limit);
    }
    if (res.initial_fuel > 0.0) {
      std::format_to(std::back_inserter(out), "Fuel recovery: {:.0f}.\n",
                     res.initial_fuel);
      if (res.fuel_room_limit.has_value()) {
        std::format_to(std::back_inserter(out),
                       "(There is only room for {:.2f} fuel.)\n",
                       *res.fuel_room_limit);
      }
    }
    if (res.initial_destruct > 0) {
      std::format_to(std::back_inserter(out), "Weapons recovery: {}.\n",
                     res.initial_destruct);
      if (res.destruct_room_limit.has_value()) {
        std::format_to(std::back_inserter(out),
                       "(There is only room for {} destruct.)\n",
                       *res.destruct_room_limit);
      }
    }
    if (res.initial_popn + res.initial_troops > 0 &&
        !res.foreign_sector_blocks_crew) {
      std::format_to(std::back_inserter(out),
                     "Population/Troops recovery: {}/{}.\n", res.initial_popn,
                     res.initial_troops);
      if (res.troops_room_limit.has_value()) {
        std::format_to(std::back_inserter(out),
                       "(There is only room for {} troops.)\n",
                       *res.troops_room_limit);
      }
      if (res.crew_room_limit.has_value()) {
        std::format_to(std::back_inserter(out),
                       "(There is only room for {} crew.)\n",
                       *res.crew_room_limit);
      }
    }
    if (res.initial_crystals > 0 && !res.foreign_sector_blocks_crystals) {
      if (res.crystals_room_limit.has_value()) {
        std::format_to(std::back_inserter(out),
                       "(There is only room for {} crystals.)\n",
                       *res.crystals_room_limit);
      }
      std::format_to(std::back_inserter(out), "Crystal recovery: {}.\n",
                     res.recovered_crystals);
    }
  }
  if (res.colonized_sector.has_value()) {
    std::format_to(std::back_inserter(out), "Sector {} Colonized.\n",
                   *res.colonized_sector);
  }
  if (res.was_landed) {
    out += "\nScrapped.\n";
  } else {
    out += "\nDestroyed.\n";
  }
  return out;
}

/// Abstract base class for polymorphic UI presentation across wire protocols.
class Presenter {
public:
  virtual ~Presenter() = default;

  [[nodiscard]] virtual std::string render(const TripEstimate& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const PlanetMapViewModel& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const CreatedShipSummary& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const InitializedShipReport& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const CapturedShipsReport& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const ShipOrdersHeader& vm) const = 0;
  [[nodiscard]] virtual std::string render(const ShipOrderStatus& vm) const = 0;
  [[nodiscard]] virtual std::string render(const OrderUpdate& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const ReactorOverloadEvent& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const MechAttackPeopleResult& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const PeopleAttackMechResult& vm) const = 0;
  [[nodiscard]] virtual std::string render(const ShipShotResult& vm) const = 0;
  [[nodiscard]] virtual std::string render(const BombardResult& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const MineDetonationReport& vm) const = 0;
  [[nodiscard]] virtual std::string render(const JettisonResult& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const MountCrystalResult& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const DismountCrystalResult& vm) const = 0;
  [[nodiscard]] virtual std::string render(const GrantShipResult& vm) const = 0;
  [[nodiscard]] virtual std::string render(const ScrapShipResult& vm) const = 0;
};

/// Server-rendered ASCII/ANSI terminal presenter for Telnet sessions.
class AsciiPresenter final : public Presenter {
public:
  [[nodiscard]] std::string render(const TripEstimate& vm) const override {
    return render_trip_estimate(vm);
  }

  [[nodiscard]] std::string
  render(const PlanetMapViewModel& vm) const override {
    return render_ascii_planet_map(vm);
  }

  [[nodiscard]] std::string
  render(const CreatedShipSummary& vm) const override {
    return render_created_ship_summary(vm);
  }

  [[nodiscard]] std::string
  render(const InitializedShipReport& vm) const override {
    return render_initialized_ship_report(vm);
  }

  [[nodiscard]] std::string
  render(const CapturedShipsReport& vm) const override {
    return render_captured_ships_report(vm);
  }

  [[nodiscard]] std::string render(const ShipOrdersHeader& vm) const override {
    return render_ship_orders_header(vm);
  }

  [[nodiscard]] std::string render(const ShipOrderStatus& vm) const override {
    return render_ship_order_status(vm);
  }

  [[nodiscard]] std::string render(const OrderUpdate& vm) const override {
    return render_order_update(vm);
  }

  [[nodiscard]] std::string
  render(const ReactorOverloadEvent& vm) const override {
    return render_reactor_overload_event(vm);
  }

  [[nodiscard]] std::string
  render(const MechAttackPeopleResult& vm) const override {
    return render_mech_attack_people_long(vm);
  }

  [[nodiscard]] std::string
  render(const PeopleAttackMechResult& vm) const override {
    return render_people_attack_mech_long(vm);
  }

  [[nodiscard]] std::string render(const ShipShotResult& vm) const override {
    return render_ship_shot_long(vm);
  }

  [[nodiscard]] std::string render(const BombardResult& vm) const override {
    return render_bombard_long(vm);
  }

  [[nodiscard]] std::string
  render(const MineDetonationReport& vm) const override {
    return render_mine_detonation_report(vm);
  }

  [[nodiscard]] std::string render(const JettisonResult& vm) const override {
    return render_jettison_result(vm);
  }

  [[nodiscard]] std::string
  render(const MountCrystalResult& vm) const override {
    return render_mount_crystal_result(vm);
  }

  [[nodiscard]] std::string
  render(const DismountCrystalResult& vm) const override {
    return render_dismount_crystal_result(vm);
  }

  [[nodiscard]] std::string render(const GrantShipResult& vm) const override {
    return render_grant_ship_result(vm);
  }

  [[nodiscard]] std::string render(const ScrapShipResult& vm) const override {
    return render_scrap_ship_result(vm);
  }
};

/// Structured Glaze JSON-lines presenter for modern rich/TUI/GUI clients.
class JsonPresenter final : public Presenter {
public:
  [[nodiscard]] std::string render(const TripEstimate& vm) const override {
    return render_json_envelope("trip_estimate", vm);
  }

  [[nodiscard]] std::string
  render(const PlanetMapViewModel& vm) const override {
    return render_json_planet_map(vm);
  }

  [[nodiscard]] std::string
  render(const CreatedShipSummary& vm) const override {
    return render_json_envelope("created_ship", vm);
  }

  [[nodiscard]] std::string
  render(const InitializedShipReport& vm) const override {
    return render_json_envelope("initialized_ship", vm);
  }

  [[nodiscard]] std::string
  render(const CapturedShipsReport& vm) const override {
    return vm.captured_ships.empty()
               ? std::string{}
               : render_json_envelope("captured_ships", vm);
  }

  [[nodiscard]] std::string render(const ShipOrdersHeader&) const override {
    return std::string{};
  }

  [[nodiscard]] std::string render(const ShipOrderStatus& vm) const override {
    return render_json_envelope("ship_order_status", vm);
  }

  [[nodiscard]] std::string render(const OrderUpdate& vm) const override {
    return vm.notice == OrderUpdateNotice::None
               ? std::string{}
               : render_json_envelope("order_update", vm);
  }

  [[nodiscard]] std::string
  render(const ReactorOverloadEvent& vm) const override {
    return render_json_envelope("reactor_overload", vm);
  }

  [[nodiscard]] std::string
  render(const MechAttackPeopleResult& vm) const override {
    return render_json_envelope("mech_attack_people", vm);
  }

  [[nodiscard]] std::string
  render(const PeopleAttackMechResult& vm) const override {
    return render_json_envelope("people_attack_mech", vm);
  }

  [[nodiscard]] std::string render(const ShipShotResult& vm) const override {
    return render_json_envelope("ship_shot", vm);
  }

  [[nodiscard]] std::string render(const BombardResult& vm) const override {
    return render_json_envelope("bombard_result", vm);
  }

  [[nodiscard]] std::string
  render(const MineDetonationReport& vm) const override {
    return render_json_envelope("mine_detonation", vm);
  }

  [[nodiscard]] std::string render(const JettisonResult& vm) const override {
    return render_json_envelope("jettison", vm);
  }

  [[nodiscard]] std::string
  render(const MountCrystalResult& vm) const override {
    return render_json_envelope("mount_crystal", vm);
  }

  [[nodiscard]] std::string
  render(const DismountCrystalResult& vm) const override {
    return render_json_envelope("dismount_crystal", vm);
  }

  [[nodiscard]] std::string render(const GrantShipResult& vm) const override {
    return render_json_envelope("grant_ship", vm);
  }

  [[nodiscard]] std::string render(const ScrapShipResult& vm) const override {
    return render_json_envelope("scrap_ship", vm);
  }
};

/// Returns the stateless singleton `Presenter` corresponding to `mode`.
[[nodiscard]] inline const Presenter& presenter_for(UiMode mode) noexcept {
  static const AsciiPresenter ascii;
  static const JsonPresenter json;
  if (mode == UiMode::JSON) {
    return json;
  }
  return ascii;
}

/// Polymorphically formats `vm` according to `mode` and writes it to `out`.
/// Found via ADL from `GameObj::present(const ViewModel&)` because `UiMode`
/// resides in `namespace GB::presentation`.
template <typename ViewModel>
void present_to(std::ostream& out, UiMode mode, const ViewModel& vm) {
  out << presenter_for(mode).render(vm);
}

}  // namespace GB::presentation
