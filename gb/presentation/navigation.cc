// SPDX-License-Identifier: Apache-2.0

/// \file navigation.cc
/// \brief Implementation of navigation, trip estimate, ship capture, and ship
/// order rendering.

module gb.presentation.navigation;

import gb.presentation.core;
import glaze.core;
import glaze.json;
import gb.repositories.glaze;
import strong_id;

namespace GB::presentation {

std::string render_trip_estimate(const TripEstimate& est) {
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

std::string render_captured_ships_report(const CapturedShipsReport& report) {
  std::string out;
  for (const auto& ship : report.captured_ships) {
    std::format_to(std::back_inserter(out), "{} CAPTURED!\n",
                   ship.ship_display);
  }
  return out;
}

std::string format_order_error(const OrderError& err) {
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

std::string render_order_update(const OrderUpdate& update) {
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

std::string render_ship_orders_header(const ShipOrdersHeader&) {
  return "    #       name       sp orbits     destin     options\n";
}

std::string render_ship_order_status(const ShipOrderStatus& status) {
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

std::string render_for_mode(UiMode mode, const TripEstimate& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("trip_estimate", vm);
  }
  return render_trip_estimate(vm);
}

std::string render_for_mode(UiMode mode, const CapturedShipsReport& vm) {
  if (mode == UiMode::JSON) {
    return vm.captured_ships.empty()
               ? std::string{}
               : render_json_envelope("captured_ships", vm);
  }
  return render_captured_ships_report(vm);
}

std::string render_for_mode(UiMode mode, const ShipOrdersHeader& vm) {
  if (mode == UiMode::JSON) {
    return std::string{};
  }
  return render_ship_orders_header(vm);
}

std::string render_for_mode(UiMode mode, const ShipOrderStatus& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("ship_order_status", vm);
  }
  return render_ship_order_status(vm);
}

std::string render_for_mode(UiMode mode, const OrderUpdate& vm) {
  if (mode == UiMode::JSON) {
    return vm.notice == OrderUpdateNotice::None
               ? std::string{}
               : render_json_envelope("order_update", vm);
  }
  return render_order_update(vm);
}

std::string render_proj_fuel(const ProjFuelViewModel& vm) {
  std::string out = std::format(
      "\n  ----- ===== FUEL ESTIMATES ===== ----\n\nAt Current Fuel "
      "Cargo ({:.2f}f):\n",
      vm.current_fuel_cargo);
  if (!vm.current_estimate) {
    out += "The ship will not be able to complete the trip.\n";
  } else {
    out += render_trip_estimate(*vm.current_estimate);
  }

  std::format_to(std::back_inserter(out), "At Optimum Fuel Level ({:.2f}f):\n",
                 vm.optimum_fuel_level);
  if (!vm.optimum_estimate) {
    out += "The ship will not be able to complete the trip.\n";
  } else {
    out += render_trip_estimate(*vm.optimum_estimate);
  }
  return out;
}

std::string render_json_proj_fuel(const ProjFuelViewModel& vm) {
  return render_json_envelope("fuel", vm);
}

std::string render_for_mode(UiMode mode, const ProjFuelViewModel& vm) {
  return mode == UiMode::JSON ? render_json_proj_fuel(vm)
                              : render_proj_fuel(vm);
}

std::string
render_alien_sector_assault(const AlienSectorAssaultResult& assault) {
  std::string out = "That sector is already occupied by another player!\n";
  if (assault.requires_unload) {
    out += "You have to unload to assault alien sectors.\n";
    return out;
  }

  const std::string_view pop_label =
      (assault.attacker_type == PopulationType::CIV) ? "civ" : "mil";
  std::format_to(std::back_inserter(out), "{} {} unloaded...\n",
                 assault.unloaded_people, pop_label);
  std::format_to(std::back_inserter(out), "Crew compliment {} civ  {} mil\n",
                 assault.ship_civ_after_unload, assault.ship_mil_after_unload);
  std::format_to(std::back_inserter(out), "{} {} assault {} civ/{} mil\n",
                 assault.unloaded_people, pop_label,
                 assault.initial_defender_civ, assault.initial_defender_mil);
  std::format_to(std::back_inserter(out), "Attack: {:.2f}   Defense: {:.2f}.\n",
                 assault.attack_strength, assault.defense_strength);

  if (assault.defender_wiped_out) {
    if (assault.attacker_absorbed_bodies) {
      std::format_to(std::back_inserter(out), "{} alien bodies absorbed.\n",
                     assault.absorbed_bodies);
    }
  } else {
    if (assault.defender_absorbed_bodies) {
      std::format_to(std::back_inserter(out),
                     "Metamorphs have absorbed {} bodies!!!\n",
                     assault.absorbed_bodies);
    }
    std::format_to(std::back_inserter(out), "Loading {} {}\n",
                   assault.surviving_attackers, pop_label);
  }

  if (assault.attacker_won) {
    out += "VICTORY! The sector is yours!\n";
    if (assault.surviving_attackers > 0) {
      std::format_to(std::back_inserter(out), "{} {} move in.\n",
                     assault.surviving_attackers,
                     assault.attacker_type == PopulationType::CIV ? "civilians"
                                                                  : "troops");
    }
  } else {
    out += "DEFEAT!  Your assault was repulsed.\n";
  }

  std::format_to(std::back_inserter(out),
                 "Crew casualties: Yours: {} {}    Theirs: {} mil/{} civ\n",
                 assault.attacker_casualties, pop_label,
                 assault.defender_mil_casualties,
                 assault.defender_civ_casualties);
  return out;
}

std::optional<std::string> render_alien_assault_absorption_telegram(
    const AlienSectorAssaultResult& assault) {
  if (assault.requires_unload) {
    return std::nullopt;
  }
  if (assault.defender_wiped_out && assault.attacker_absorbed_bodies) {
    return std::format("Metamorphs have absorbed {} bodies!!!\n",
                       assault.absorbed_bodies);
  }
  if (!assault.defender_wiped_out && assault.defender_absorbed_bodies) {
    return std::format("{} alien bodies absorbed.\n", assault.absorbed_bodies);
  }
  return std::nullopt;
}

std::string render_alien_assault_defender_telegram(
    const AlienSectorAssaultResult& assault) {
  std::string telegram = std::format(
      "/{}/{}: {} [{}] {} assaults {} [{}] {}({}) {}\n", assault.star_name,
      assault.planet_name, assault.attacker_race_name, assault.attacker_player,
      assault.ship_display, assault.defender_race_name, assault.defender_player,
      assault.sector_condition_symbol, assault.sector_coords,
      assault.attacker_won ? "VICTORY" : "DEFEAT");
  if (assault.attacker_won) {
    telegram += "Sector CAPTURED!\n";
  } else {
    telegram += "Assault repulsed!\n";
  }
  telegram +=
      std::format("Casualties: Yours: {} mil/{} civ    Theirs: {} {}\n",
                  assault.defender_mil_casualties,
                  assault.defender_civ_casualties, assault.attacker_casualties,
                  assault.attacker_type == PopulationType::MIL ? "mil" : "civ");
  return telegram;
}

std::string render_alien_assault_news(const AlienSectorAssaultResult& assault) {
  return std::format("/{}/{}: {} [{}] {} {} by {} [{}] on sector {}.\n",
                     assault.star_name, assault.planet_name,
                     assault.attacker_race_name, assault.attacker_player,
                     assault.ship_display,
                     assault.attacker_won ? "CAPTURED" : "failed to capture",
                     assault.defender_race_name, assault.defender_player,
                     assault.sector_coords);
}

namespace {

void append_cargo_transfer_summary(std::string& out, ShipCargoType commodity,
                                   std::int64_t amount,
                                   std::string_view ship_display,
                                   population_t resulting_popn,
                                   population_t resulting_troops,
                                   bool check_boobytrap, bool is_boobytrapped) {
  switch (commodity) {
    case ShipCargoType::Crew:
      std::format_to(std::back_inserter(out),
                     "crew complement of {} is now {}.\n", ship_display,
                     resulting_popn);
      break;
    case ShipCargoType::Troops:
      std::format_to(std::back_inserter(out),
                     "troop complement of {} is now {}.\n", ship_display,
                     resulting_troops);
      break;
    case ShipCargoType::Destruct:
      std::format_to(std::back_inserter(out), "{} destruct transferred.\n",
                     amount);
      if (check_boobytrap) {
        std::format_to(std::back_inserter(out), "\n{} {}\n", ship_display,
                       is_boobytrapped ? "now boobytrapped."
                                       : "no longer boobytrapped.");
      }
      break;
    case ShipCargoType::Crystal:
      std::format_to(std::back_inserter(out), "{} crystal(s) transferred.\n",
                     amount);
      break;
    case ShipCargoType::Fuel:
      std::format_to(std::back_inserter(out), "{} fuel transferred.\n", amount);
      break;
    case ShipCargoType::Resource:
      std::format_to(std::back_inserter(out), "{} resources transferred.\n",
                     amount);
      break;
  }
}

}  // namespace

std::string render_planet_cargo_transfer(const PlanetCargoTransferResult& res) {
  if (res.assault.has_value()) {
    return render_alien_sector_assault(*res.assault);
  }

  std::string out;
  switch (res.sector_event) {
    case PlanetSectorEvent::None:
      break;
    case PlanetSectorEvent::Colonized:
      std::format_to(std::back_inserter(out), "sector {} COLONIZED.\n",
                     res.sector_coords);
      break;
    case PlanetSectorEvent::Occupied:
      std::format_to(std::back_inserter(out), "sector {} OCCUPIED.\n",
                     res.sector_coords);
      break;
    case PlanetSectorEvent::Evacuated:
      std::format_to(std::back_inserter(out), "sector {} evacuated.\n",
                     res.sector_coords);
      break;
  }

  append_cargo_transfer_summary(
      out, res.commodity, res.amount, res.ship_display, res.resulting_popn,
      res.resulting_troops, res.check_boobytrap, res.is_boobytrapped);
  return out;
}

std::string
render_json_planet_cargo_transfer(const PlanetCargoTransferResult& res) {
  return render_json_envelope("planet_cargo_transfer", res);
}

std::string render_for_mode(UiMode mode, const PlanetCargoTransferResult& vm) {
  return mode == UiMode::JSON ? render_json_planet_cargo_transfer(vm)
                              : render_planet_cargo_transfer(vm);
}

std::string format_ship_docking_error(const ShipDockingError& err) {
  switch (err.reason) {
    case ShipDockingErrorReason::Inactive:
      return std::format("{} is irradiated and inactive.\n", err.ship_display);
    case ShipDockingErrorReason::NotLandedOrDocked:
      return std::format("{} is not landed or docked.\n", err.ship_display);
    case ShipDockingErrorReason::WrongPlanetScope:
      return std::format(
          "{} at {}\nChange scope to the planet this ship is landed on.\n",
          err.ship_display, err.land_coords);
    case ShipDockingErrorReason::NotDocked:
      return std::format("{} is not docked.\n", err.ship_display);
    case ShipDockingErrorReason::TargetNotDocked:
      return std::format("{} is not docked.\n", err.target_ship_display);
    case ShipDockingErrorReason::TargetOverloaded:
      return std::format("{} is overloaded!\n", err.target_ship_display);
    case ShipDockingErrorReason::DestinationBogus:
      return "Destination ship is bogus.\n";
  }
  std::unreachable();
}

std::string render_ship_docking_status(const ShipDockingStatus& status) {
  if (!status.is_docked_to_ship) {
    return std::format("{} at {}\n", status.ship_display, status.land_coords);
  }
  std::string out = std::format("{} docked with {}\n", status.ship_display,
                                status.target_ship_display);
  if (status.is_different_owner) {
    std::format_to(std::back_inserter(out), "Player {} owns that ship.\n",
                   status.target_owner);
  }
  return out;
}

std::string render_json_ship_docking_status(const ShipDockingStatus& status) {
  return render_json_envelope("ship_docking_status", status);
}

std::string render_for_mode(UiMode mode, const ShipDockingStatus& vm) {
  return mode == UiMode::JSON ? render_json_ship_docking_status(vm)
                              : render_ship_docking_status(vm);
}

std::string render_ship_cargo_transfer(const ShipCargoTransferResult& res) {
  std::string out;
  append_cargo_transfer_summary(
      out, res.commodity, res.amount, res.ship_display, res.resulting_popn,
      res.resulting_troops, res.check_boobytrap, res.is_boobytrapped);
  return out;
}

std::string
render_json_ship_cargo_transfer(const ShipCargoTransferResult& res) {
  return render_json_envelope("ship_cargo_transfer", res);
}

std::string render_for_mode(UiMode mode, const ShipCargoTransferResult& vm) {
  return mode == UiMode::JSON ? render_json_ship_cargo_transfer(vm)
                              : render_ship_cargo_transfer(vm);
}

std::optional<std::string>
render_ship_cargo_transfer_telegram(const ShipCargoTransferResult& res) {
  if (!res.notify_target_owner) {
    return std::nullopt;
  }
  std::string tele_lines;
  const auto abs_amt = std::abs(res.amount);
  switch (res.commodity) {
    case ShipCargoType::Resource:
      tele_lines = std::format("{} Resources\n", abs_amt);
      break;
    case ShipCargoType::Fuel:
      tele_lines = std::format("{} Fuel\n", abs_amt);
      break;
    case ShipCargoType::Destruct:
      tele_lines = std::format("{} Destruct\n", abs_amt);
      break;
    case ShipCargoType::Crystal:
      tele_lines = std::format("{} Crystal(s)\n", abs_amt);
      break;
    case ShipCargoType::Crew:
    case ShipCargoType::Troops:
      tele_lines =
          std::format("{} {}\n", abs_amt,
                      res.is_metamorph ? "tons of biomass" : "population");
      break;
  }
  return std::format(
      "Audio-vibatory-physio-molecular transport device #{} gave "
      "your ship {} the following:\n{}",
      res.ship_display, res.target_ship_display, tele_lines);
}

std::string format_transporter_error(TransporterError err) {
  switch (err) {
    case TransporterError::OriginNotLanded:
      return "Origin ship not landed.\n";
    case TransporterError::WrongPlanetScope:
      return "Change scope to the planet the ship is landed on!\n";
    case TransporterError::OriginDamaged:
      return "Origin device is damaged.\n";
    case TransporterError::HopperBlocked:
      return "The hopper seems to be blocked.\n";
    case TransporterError::TargetNotReceiving:
      return "The target device is not receiving.\n";
    case TransporterError::TargetNotLanded:
      return "Target ship not landed.\n";
    case TransporterError::TargetDamaged:
      return "Target device is damaged.\n";
  }
  std::unreachable();
}

std::string render_transporter_transfer(const TransporterTransferResult& res) {
  std::string out = "Zap\07!\n";
  for (const auto& item : res.transfers) {
    std::string_view label;
    switch (item.commodity) {
      case ShipCargoType::Resource:
        label = "resources";
        break;
      case ShipCargoType::Fuel:
        label = "fuel";
        break;
      case ShipCargoType::Destruct:
        label = "destruct";
        break;
      case ShipCargoType::Crew:
        label = "population";
        break;
      case ShipCargoType::Troops:
        label = "troops";
        break;
      case ShipCargoType::Crystal:
        label = "crystal(s)";
        break;
    }
    std::format_to(std::back_inserter(out), "{} {} transferred.\n", item.amount,
                   label);
  }
  return out;
}

std::string
render_json_transporter_transfer(const TransporterTransferResult& res) {
  return render_json_envelope("transporter_transfer", res);
}

std::string render_for_mode(UiMode mode, const TransporterTransferResult& vm) {
  return mode == UiMode::JSON ? render_json_transporter_transfer(vm)
                              : render_transporter_transfer(vm);
}

std::optional<std::string>
render_transporter_telegram(const TransporterTransferResult& res) {
  if (!res.notify_receiver) {
    return std::nullopt;
  }
  std::string tele_lines;
  for (const auto& item : res.transfers) {
    std::string_view tele_label;
    switch (item.commodity) {
      case ShipCargoType::Resource:
        tele_label = "Resources";
        break;
      case ShipCargoType::Fuel:
        tele_label = "Fuel";
        break;
      case ShipCargoType::Destruct:
        tele_label = "Destruct";
        break;
      case ShipCargoType::Crew:
        tele_label = res.is_metamorph ? "tons of biomass" : "population";
        break;
      case ShipCargoType::Troops:
        tele_label = "troops";
        break;
      case ShipCargoType::Crystal:
        tele_label = "crystal(s)";
        break;
    }
    std::format_to(std::back_inserter(tele_lines), "{} {}\n", item.amount,
                   tele_label);
  }
  return std::format("Audio-vibatory-physio-molecular transport device #{} "
                     "gave your ship {} the following:\n{}",
                     res.sender_ship_display, res.receiver_ship_display,
                     tele_lines);
}

}  // namespace GB::presentation
