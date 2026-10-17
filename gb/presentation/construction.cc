// SPDX-License-Identifier: Apache-2.0

/// \file construction.cc
/// \brief Implementation of ship construction presentation formatters.

module gb.presentation.construction;

import gb.presentation.core;
import glaze.core;
import glaze.json;
import gb.repositories.glaze;
import strong_id;
import tabulate;

namespace GB::presentation {

namespace {

std::string format_factory_guns(const FactorySpecsViewModel& vm) {
  std::string guns = "Guns:";
  if (vm.has_primary && vm.primary_battery.has_guns()) {
    guns += std::format("{:3}{:c}", vm.primary_battery.count,
                        caliber_char(vm.primary_battery.caliber));
  }
  if (vm.has_secondary && vm.secondary_battery.has_guns()) {
    guns += std::format("/{:}{:c}", vm.secondary_battery.count,
                        caliber_char(vm.secondary_battery.caliber));
  }
  return guns;
}

std::string render_factory_design_stats(const FactoryDesignStats& stats) {
  std::string out;
  std::format_to(std::back_inserter(out),
                 "The current cost of the ship is {} resources.\n"
                 "The current base mass of the ship is {:.1f} - size is {}.\n"
                 "Ship complexity is {:.1f} (you have {:.1f} engineering "
                 "technology).\n",
                 stats.build_cost, stats.base_mass, stats.design_size,
                 stats.complexity, stats.race_tech);
  return out;
}

}  // namespace

std::string format_planet_build_error(const PlanetBuildError& err) {
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

std::string format_ship_build_error(ShipBuildError err) {
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

std::string render_created_ship_summary(const CreatedShipSummary& summary) {
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

std::string
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

std::string render_for_mode(UiMode mode, const CreatedShipSummary& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("created_ship", vm);
  }
  return render_created_ship_summary(vm);
}

std::string render_for_mode(UiMode mode, const InitializedShipReport& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("initialized_ship", vm);
  }
  return render_initialized_ship_report(vm);
}

std::string format_factory_make_error(FactoryMakeError err) {
  switch (err) {
    case FactoryMakeError::IllegalShipLetter:
      return "Illegal ship letter.\n";
    case FactoryMakeError::GodOnlyShip:
      return "Nice try!\n";
    case FactoryMakeError::DoesNotRequireFactory:
      return "This kind of ship does not require a factory to construct.\n";
    case FactoryMakeError::MaxCostExceeded:
      return "Woah!! YOU CHEATER!!!  The max cost allowed "
             "is 65535!!! I'm Telllllllling!!!\n";
  }
  std::unreachable();
}

std::string format_factory_modify_error(FactoryModifyError err) {
  switch (err) {
    case FactoryModifyError::NoShipDesignSpecified:
      return "No ship design specified. Use 'make <ship type>' first.\n";
    case FactoryModifyError::NoCharacteristicSpecified:
      return "You have to specify the characteristic you wish to modify.\n";
    case FactoryModifyError::OnlyHyperdriveModifiable:
      return "You may only modify hyperdrive installation on this kind of "
             "ship.\n";
    case FactoryModifyError::CannotModifyShip:
      return "Sorry, but you can't modify this ship right now.\n";
    case FactoryModifyError::InvalidGunCharacteristic:
      return "No such gun characteristic.\n";
    case FactoryModifyError::RidiculousSetting:
      return "That's a ridiculous setting.\n";
    case FactoryModifyError::InvalidCaliber:
      return "No such caliber.\n";
    case FactoryModifyError::NoCewDiscovery:
      return "Your race does not understand confined energy weapons.\n";
    case FactoryModifyError::InvalidCewOption:
      return "No such option for CEWs.\n";
    case FactoryModifyError::NoLaserDiscovery:
      return "Your race does not understand lasers yet.\n";
    case FactoryModifyError::InvalidCharacteristic:
      return "That characteristic either doesn't exist or can't be modified.\n";
    case FactoryModifyError::MaxCostExceeded:
      return "Woah!! YOU CHEATER!!!  The max cost allowed "
             "is 65535!!! I'm Telllllllling!!!\n";
  }
  std::unreachable();
}

std::string render_factory_specs(const FactorySpecsViewModel& vm) {
  std::ostringstream ss;
  ss << "  --- Current Production Specifications ---\n";

  tabulate::Table table;
  table.format().hide_border().column_separator("    ");

  table.add_row({vm.is_online ? "Online" : "Offline",
                 std::format("Armor:    {:4}", vm.armor),
                 format_factory_guns(vm)});
  table.add_row({std::format("Ship:  {}", vm.ship_name),
                 std::format("Crew:     {:4}", vm.max_crew),
                 vm.can_mount
                     ? std::format("Xtal Mount: {}", vm.mount ? "yes" : "no")
                     : ""});
  table.add_row({std::format("Class: {}", vm.ship_class),
                 std::format("Fuel:     {:4}", vm.max_fuel),
                 vm.can_hyperjump ? std::format("Hyperdrive: {}",
                                                vm.hyperdrive ? "yes" : "no")
                                  : ""});
  table.add_row({std::format("Cost:  {} r", vm.build_cost),
                 std::format("Cargo:    {:4}", vm.max_resource),
                 vm.can_mount_laser
                     ? std::format("Combat Lasers: {}", vm.laser ? "yes" : "no")
                     : ""});
  table.add_row(
      {std::format("Mass:  {:.1f}", vm.base_mass),
       std::format("Hanger:   {:4}", vm.max_hanger),
       vm.has_cew ? std::format("CEW: {}", vm.cew ? "yes" : "no") : ""});
  table.add_row({std::format("Size:  {}", vm.design_size),
                 std::format("Destruct: {:4}", vm.max_destruct),
                 (vm.has_cew && vm.cew)
                     ? std::format("   Opt Range: {:4}", vm.cew_range)
                     : ""});
  table.add_row(
      {std::format("Tech:  {:.1f} ({:.1f})", vm.complexity, vm.race_tech),
       std::format("Speed:    {:4}", vm.max_speed),
       (vm.has_cew && vm.cew) ? std::format("   Energy:    {:4d}", vm.cew)
                              : ""});

  ss << table << "\n";

  if (vm.race_tech < vm.complexity) {
    ss << "Your engineering capability is not "
          "advanced enough to produce this "
          "design.\n";
  }
  return ss.str();
}

std::string render_json_factory_specs(const FactorySpecsViewModel& vm) {
  return render_json_envelope("factory_specs", vm);
}

std::string render_factory_make_result(const FactoryMakeResult& vm) {
  std::string out;
  std::format_to(std::back_inserter(out),
                 "Factory designated to produce {}s.\n"
                 "Design complexity {:.1f} ({:.1f}).\n",
                 vm.ship_name, vm.stats.complexity, vm.stats.race_tech);
  if (vm.stats.complexity > vm.stats.race_tech) {
    out += "You can't produce this design yet!\n";
  }
  out += render_factory_design_stats(vm.stats);
  return out;
}

std::string render_json_factory_make_result(const FactoryMakeResult& vm) {
  return render_json_envelope("factory_make", vm);
}

std::string render_factory_modify_result(const FactoryModifyResult& vm) {
  return render_factory_design_stats(vm.stats);
}

std::string render_json_factory_modify_result(const FactoryModifyResult& vm) {
  return render_json_envelope("factory_modify", vm);
}

std::string render_for_mode(UiMode mode, const FactorySpecsViewModel& vm) {
  if (mode == UiMode::JSON) {
    return render_json_factory_specs(vm);
  }
  return render_factory_specs(vm);
}

std::string render_for_mode(UiMode mode, const FactoryMakeResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_factory_make_result(vm);
  }
  return render_factory_make_result(vm);
}

std::string render_for_mode(UiMode mode, const FactoryModifyResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_factory_modify_result(vm);
  }
  return render_factory_modify_result(vm);
}

}  // namespace GB::presentation
