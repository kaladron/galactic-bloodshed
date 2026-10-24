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
  std::string out = "  --- Current Production Specifications ---\n";

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

  std::format_to(std::back_inserter(out), "{}\n", table.str());

  if (vm.race_tech < vm.complexity) {
    out += "Your engineering capability is not "
           "advanced enough to produce this "
           "design.\n";
  }
  return out;
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

namespace {

std::array<ShipType, NUMSTYPES> get_sorted_ship_types() {
  std::array<ShipType, NUMSTYPES> result{};
  for (int i = 0; i < NUMSTYPES; ++i) {
    result[i] = static_cast<ShipType>(i);
  }
  std::ranges::sort(result, [](ShipType a, ShipType b) {
    return complexity(a) < complexity(b);
  });
  return result;
}

tabulate::Table create_ship_spec_table() {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");

  table.column(0).format().width(1);   // Letter
  table.column(1).format().width(15);  // Name
  table.column(2).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(3).format().width(5).font_align(tabulate::FontAlign::right);
  table.column(4).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(5).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(6).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(7).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(8).format().width(3).font_align(tabulate::FontAlign::right);
  table.column(9).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(10).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(11).format().width(2).font_align(tabulate::FontAlign::right);
  table.column(12).format().width(4).font_align(tabulate::FontAlign::right);
  table.column(13).format().width(4).font_align(tabulate::FontAlign::right);

  table.add_row({"?", "name", "cargo", "hang", "arm", "dest", "gun", "pri",
                 "sec", "fuel", "crew", "sp", "tech", "cost"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  return table;
}

void add_ship_spec_row(tabulate::Table& table, const ShipSpecRow& row) {
  table.add_row(
      {row.letter, row.name, std::format("{}", row.max_resource),
       std::format("{}", row.max_hangar), std::format("{}", row.base_armor),
       std::format("{}", row.max_destruct), std::format("{}", row.max_guns),
       row.primary_caliber, row.secondary_caliber,
       std::format("{}", row.max_fuel), std::format("{}", row.max_crew),
       std::format("{}", row.base_speed), std::format("{:.0f}", row.base_tech),
       std::format("{}", row.cost)});
}

}  // namespace

ShipSpecRow build_ship_spec_row(ShipType type, const Race& race) {
  const auto& tmpl = ship_template(type);
  return ShipSpecRow{
      .letter = std::string(1, tmpl.letter),
      .name = std::string(tmpl.name),
      .max_resource = tmpl.max_resource,
      .max_hangar = tmpl.max_hangar,
      .base_armor = tmpl.base_armor,
      .max_destruct = tmpl.max_destruct,
      .max_guns = tmpl.max_guns,
      .primary_caliber =
          std::format("{}", gun_caliber(tmpl.max_primary_caliber)),
      .secondary_caliber =
          std::format("{}", gun_caliber(tmpl.max_secondary_caliber)),
      .max_fuel = tmpl.max_fuel,
      .max_crew = tmpl.max_crew,
      .base_speed = tmpl.base_speed,
      .base_tech = tmpl.base_tech,
      .cost = Shipcost(type, race),
  };
}

ShipSpecsTableViewModel build_ship_specs_table_vm(const Race& race) {
  ShipSpecsTableViewModel vm;
  const auto sorted_ships = get_sorted_ship_types();
  for (ShipType i : sorted_ships) {
    const auto& tmpl = ship_template(i);
    if ((!tmpl.is_god_only || race.God) &&
        (race.pods || i != ShipType::STYPE_POD) && tmpl.is_programmed) {
      vm.rows.push_back(build_ship_spec_row(i, race));
    }
  }
  return vm;
}

ShipSpecDetailViewModel
build_ship_spec_detail_vm(ShipType type, const Race& race,
                          std::optional<std::string_view> description) {
  const auto& tmpl = ship_template(type);
  ShipSpecDetailViewModel vm{
      .description = (description && !description->empty())
                         ? std::optional<std::string>{*description}
                         : std::nullopt,
      .can_build_on_planet = tmpl.can_build_on_planet(),
      .spec = build_ship_spec_row(type, race),
  };
  for (int j = 0; j < NUMSTYPES; ++j) {
    const auto& builder_tmpl = ship_template(ShipType{j});
    if (tmpl.can_be_built_by(builder_tmpl)) {
      vm.builder_letters.emplace_back(1, builder_tmpl.letter);
    }
  }
  return vm;
}

std::string render_ship_specs_table(const ShipSpecsTableViewModel& vm) {
  auto table = create_ship_spec_table();
  for (const auto& row : vm.rows) {
    add_ship_spec_row(table, row);
  }
  return std::format("     - Default ship parameters -\n{}\n", table.str());
}

std::string render_json_ship_specs_table(const ShipSpecsTableViewModel& vm) {
  return render_json_envelope("ship_specs_table", vm);
}

std::string render_ship_spec_detail(const ShipSpecDetailViewModel& vm) {
  std::string out;
  if (vm.description && !vm.description->empty()) {
    out += "\n";
    out += *vm.description;
    if (!vm.description->ends_with('\n')) {
      out += "\n";
    }
  }
  if (vm.can_build_on_planet) {
    out += "\nCan be constructed on planet.";
  }
  const std::size_t n = vm.builder_letters.size();
  if (n > 0) {
    out += "\nCan be built by ";
    for (std::size_t m = 0; m < n; ++m) {
      const std::size_t remaining = n - (m + 1);
      if (remaining > 1) {
        std::format_to(std::back_inserter(out), "{}, ", vm.builder_letters[m]);
      } else if (remaining == 1) {
        std::format_to(std::back_inserter(out), "{} and ",
                       vm.builder_letters[m]);
      } else {
        std::format_to(std::back_inserter(out), "{} ", vm.builder_letters[m]);
      }
    }
    out += "type ships.\n";
  }
  auto table = create_ship_spec_table();
  add_ship_spec_row(table, vm.spec);
  std::format_to(std::back_inserter(out), "{}\n", table.str());
  return out;
}

std::string render_json_ship_spec_detail(const ShipSpecDetailViewModel& vm) {
  return render_json_envelope("ship_spec_detail", vm);
}

std::string render_for_mode(UiMode mode, const ShipSpecsTableViewModel& vm) {
  if (mode == UiMode::JSON) {
    return render_json_ship_specs_table(vm);
  }
  return render_ship_specs_table(vm);
}

std::string render_for_mode(UiMode mode, const ShipSpecDetailViewModel& vm) {
  if (mode == UiMode::JSON) {
    return render_json_ship_spec_detail(vm);
  }
  return render_ship_spec_detail(vm);
}

std::string format_upgrade_error(const UpgradeError& err) {
  switch (err.reason) {
    case UpgradeErrorReason::ShipDamaged:
      return "You cannot upgrade damaged ships.\n";
    case UpgradeErrorReason::CannotUpgradeFactory:
      return "You can't upgrade factories.\n";
    case UpgradeErrorReason::CannotUpgradeShip:
      return "This ship cannot be upgraded.\n";
    case UpgradeErrorReason::InvalidGunCharacteristic:
      return "No such gun characteristic.\n";
    case UpgradeErrorReason::NoCaliberDefined:
      return "No caliber defined.\n";
    case UpgradeErrorReason::RidiculousSetting:
      return "That's a ridiculous setting.\n";
    case UpgradeErrorReason::InvalidCaliber:
      return "No such caliber.\n";
    case UpgradeErrorReason::NoCewDiscovery:
      return "Your race cannot build confined energy weapons.\n";
    case UpgradeErrorReason::InvalidCewOption:
      return "No such option for CEWs.\n";
    case UpgradeErrorReason::NoCrystalDiscovery:
      return "Your race does not now how to utilize crystal power yet.\n";
    case UpgradeErrorReason::NoLaserDiscovery:
      return "Your race cannot build lasers.\n";
    case UpgradeErrorReason::InvalidCharacteristic:
      return "That characteristic either doesn't exist or can't be modified.\n";
    case UpgradeErrorReason::InsufficientTech:
      return std::format(
          "This upgrade requires an engineering technology of {:.1f}.\n",
          err.required_tech);
    case UpgradeErrorReason::InsufficientCarrierHangar:
      return std::format("Not enough free hanger space on {}{}.\n"
                         "{} more needed.\n",
                         err.carrier_letter, err.carrier_shipno,
                         err.more_hangar_needed);
    case UpgradeErrorReason::CannotDowngrade:
      return "You cannot downgrade ships!\n";
    case UpgradeErrorReason::InsufficientResources:
      return std::format(
          "Old value {}r   New value {}r\n"
          "You need {} resources on board to make this modification.\n",
          err.old_cost, err.new_cost, err.net_cost);
  }
  std::unreachable();
}

std::string render_upgrade_ship_result(const UpgradeShipResult& vm) {
  return std::format("Old value {}r   New value {}r\n"
                     "Characteristic modified at a cost of {} resources.\n",
                     vm.old_cost, vm.new_cost, vm.net_cost);
}

std::string render_json_upgrade_ship_result(const UpgradeShipResult& vm) {
  return render_json_envelope("ship_upgrade", vm);
}

std::string render_for_mode(UiMode mode, const UpgradeShipResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_upgrade_ship_result(vm);
  }
  return render_upgrade_ship_result(vm);
}

}  // namespace GB::presentation
