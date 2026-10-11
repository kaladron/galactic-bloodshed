// SPDX-License-Identifier: Apache-2.0

/// \file ship_ops.cc
/// \brief Implementation of single-ship operation presentation formatters.

module gb.presentation.ship_ops;

import gb.presentation.core;
import glaze.core;
import glaze.json;
import gb.repositories.glaze;
import strong_id;
import tabulate;

namespace GB::presentation {

std::string render_detonate_error(DetonateError err) {
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

std::string render_jettison_error(const JettisonError& err) {
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

std::string render_jettison_result(const JettisonResult& res) {
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

std::string render_mount_crystal_error(MountCrystalError err) {
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

std::string render_mount_crystal_result(const MountCrystalResult&) {
  return "Mounted.\n";
}

std::string render_dismount_crystal_error(DismountCrystalError err) {
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

std::string render_dismount_crystal_result(const DismountCrystalResult& res) {
  std::string out = "Dismounted.\n";
  if (res.hyperdrive_discharged) {
    out += "Discharged.\n";
  }
  if (res.laser_deactivated) {
    out += "Laser deactivated.\n";
  }
  return out;
}

std::string render_grant_ship_result(const GrantShipResult& res) {
  return std::format("{} granted to \"{}\"\n", res.ship_display,
                     res.recipient_governor_name);
}

std::string render_grant_ship_notification(const GrantShipResult& res) {
  return std::format("\"{}\" granted you {} at {}\n", res.donor_governor_name,
                     res.ship_display, res.orbits_display);
}

std::string render_scrap_error(ScrapError err) {
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

std::string render_scrap_ship_result(const ScrapShipResult& res) {
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

std::string render_peaceful_dock_error(const PeacefulDockError& err) {
  switch (err.reason) {
    case PeacefulDockErrorReason::ShipIrradiated:
      return std::format("{} is irradiated {}% and inactive.\n",
                         err.ship_display, err.radiation);
    case PeacefulDockErrorReason::ShipAlreadyDocked:
      return std::format("{} is already docked.\n", err.ship_display);
    case PeacefulDockErrorReason::CannotDockWithSelf:
      return "You can't dock with yourself!\n";
    case PeacefulDockErrorReason::TargetNotFound:
      return "The ship wasn't found.\n";
    case PeacefulDockErrorReason::TargetNotCommandable:
      return "You are not authorized to do this.\n";
    case PeacefulDockErrorReason::NotInSameScope:
      return "Those ships are not in the same scope.\n";
    case PeacefulDockErrorReason::TargetAlreadyDocked:
      return std::format("{} is already docked.\n", err.target_display);
    case PeacefulDockErrorReason::TooFarAway:
      return std::format("{} must be {:.2f} or closer to {}.\n",
                         err.ship_display, err.max_distance,
                         err.target_display);
    case PeacefulDockErrorReason::InsufficientFuel:
      return "Not enough fuel.\n";
  }
  std::unreachable();
}

std::string render_peaceful_dock_result(const PeacefulDockResult& res) {
  std::string out = std::format(
      "Distance to {}: {:.2f}.\n"
      "This maneuver will take {:.2f} fuel (of {:.2f}.)\n\n",
      res.target_display, res.distance, res.fuel_cost, res.initial_fuel);
  if (res.hyperdrive_deactivated) {
    out += "Hyper-drive deactivated.\n";
  }
  std::format_to(std::back_inserter(out), "{} docked with {}.\n",
                 res.ship_display, res.target_display);
  return out;
}

std::string render_for_mode(UiMode mode, const JettisonResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("jettison", vm);
  }
  return render_jettison_result(vm);
}

std::string render_for_mode(UiMode mode, const MountCrystalResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("mount_crystal", vm);
  }
  return render_mount_crystal_result(vm);
}

std::string render_for_mode(UiMode mode, const DismountCrystalResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("dismount_crystal", vm);
  }
  return render_dismount_crystal_result(vm);
}

std::string render_for_mode(UiMode mode, const GrantShipResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("grant_ship", vm);
  }
  return render_grant_ship_result(vm);
}

std::string render_for_mode(UiMode mode, const ScrapShipResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("scrap_ship", vm);
  }
  return render_scrap_ship_result(vm);
}

std::string render_for_mode(UiMode mode, const PeacefulDockResult& vm) {
  if (mode == UiMode::JSON) {
    return render_json_envelope("peaceful_dock", vm);
  }
  return render_peaceful_dock_result(vm);
}

namespace {

void append_stock_table(std::string& out,
                        std::span<const RstShipEntry> entries) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
  table.add_row({"#", "name", "x", "hanger", "res", "des", "fuel", "crew/mil"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  bool has_rows = false;
  for (const auto& entry : entries) {
    if (!entry.stock) continue;
    has_rows = true;
    const auto& row = *entry.stock;
    table.add_row(
        {std::format("{}", row.number),
         std::format("{}{} {}", row.type_letter, row.crystals ? 'x' : ' ',
                     row.active ? row.name : "INACTIVE"),
         std::format("{}", row.crystals),
         std::format("{}:{}", row.hanger, row.max_hanger),
         std::format("{}:{}", row.resource, row.max_resource),
         std::format("{}:{}", row.destruct, row.max_destruct),
         std::format("{:.1f}:{}", row.fuel, row.max_fuel),
         std::format("{}/{}:{}", row.popn, row.troops, row.max_crew)});
  }

  if (has_rows) {
    std::format_to(std::back_inserter(out), "{}\n", table.str());
  }
}

void append_status_table(std::string& out,
                         std::span<const RstShipEntry> entries) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
  table.add_row({"#", "", "name", "las", "cew", "hyp", "guns", "arm", "tech",
                 "spd", "cost", "mass", "size"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  bool has_rows = false;
  for (const auto& entry : entries) {
    if (!entry.status) continue;
    has_rows = true;
    const auto& row = *entry.status;
    const std::string pod_suffix =
        row.pod_temperature ? std::format(" ({})", *row.pod_temperature) : "";
    table.add_row(
        {std::format("{}", row.number), std::format("{}", row.type_letter),
         row.active ? row.name : "INACTIVE", row.has_laser ? "yes" : "",
         row.has_cew ? "yes" : "", row.has_hyperdrive ? "yes" : "",
         row.battery_summary, std::format("{}", row.effective_armor),
         std::format("{:.0f}", row.tech), std::format("{}", row.max_speed),
         std::format("{}", row.effective_cost), std::format("{:.1f}", row.mass),
         std::format("{}{}", row.size, pod_suffix)});
  }

  if (has_rows) {
    std::format_to(std::back_inserter(out), "{}\n", table.str());
  }
}

void append_weapons_table(std::string& out,
                          std::span<const RstShipEntry> entries) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
  table.add_row(
      {"#", "", "name", "laser", "cew", "safe", "guns", "damage", "class"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  bool has_rows = false;
  for (const auto& entry : entries) {
    if (!entry.weapons) continue;
    has_rows = true;
    const auto& row = *entry.weapons;
    table.add_row(
        {std::format("{}", row.number), std::format("{}", row.type_letter),
         row.active ? row.name : "INACTIVE", row.has_laser ? "yes" : "",
         std::format("{}/{}", row.cew, row.cew_range),
         std::format("{}", row.safe_cew), row.battery_summary,
         std::format("{}%", row.damage), row.ship_class});
  }

  if (has_rows) {
    std::format_to(std::back_inserter(out), "{}\n", table.str());
  }
}

void append_factory_table(std::string& out,
                          std::span<const RstShipEntry> entries) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
  table.add_row({"#", "", "Cost", "Tech", "Mass", "Sz", "A", "Crw", "Ful",
                 "Crg", "Hng", "Dst", "Sp", "Weapons", "Lsr", "CEWs", "Range",
                 "Dmg"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  bool has_rows = false;
  for (const auto& entry : entries) {
    if (!entry.factory) continue;
    has_rows = true;
    const auto& row = *entry.factory;
    if (row.unconfigured) {
      table.add_row({std::format("{}", row.number), "", "", "", "", "", "", "",
                     "", "", "", "", "", "(No ship type specified yet)", "", "",
                     "", "75% (OFF)"});
      continue;
    }

    const std::string speed_indicator = std::format(
        "{}{}", row.has_hyperdrive ? (row.has_mount ? "+" : "*") : " ",
        row.max_speed);
    std::string damage_status = std::format("{}%", row.damage);
    if (row.damage && !row.on) {
      damage_status += "*";
    }

    table.add_row(
        {std::format("{}", row.number),
         std::format("{}", row.build_type_letter),
         std::format("{}", row.build_cost),
         std::format("{:.1f}", row.complexity),
         std::format("{:.1f}", row.base_mass), std::format("{}", row.size),
         std::format("{}", row.armor), std::format("{}", row.max_crew),
         std::format("{}", row.max_fuel), std::format("{}", row.max_resource),
         std::format("{}", row.max_hanger), std::format("{}", row.max_destruct),
         speed_indicator,
         std::format("{}/{}", row.primary_guns, row.secondary_guns),
         row.has_laser ? "yes" : " no",
         row.cew ? std::format("{}", row.cew) : "----",
         row.cew ? std::format("{}", row.cew_range) : "-----", damage_status});
  }

  if (has_rows) {
    std::format_to(std::back_inserter(out), "{}\n", table.str());
  }
}

void append_general_table(std::string& out,
                          std::span<const RstShipEntry> entries) {
  tabulate::Table table;
  table.format().hide_border().column_separator("  ");
  table.add_row({"", "#", "name", "gov", "dam", "crew", "mil", "des", "fuel",
                 "sp", "orbits", "destination"});
  table[0].format().font_style({tabulate::FontStyle::bold});

  bool has_rows = false;
  for (const auto& entry : entries) {
    if (!entry.general) continue;
    has_rows = true;
    const auto& row = *entry.general;
    const std::string name_str =
        row.active ? row.name : std::format("INACTIVE({})", row.radiation);
    const std::string speed_indicator = std::format(
        "{}{}", row.has_hyperdrive ? (row.mounted ? "+" : "*") : " ",
        row.speed);
    std::string destination;
    if (row.docked) {
      if (row.whatdest == ScopeLevel::LEVEL_SHIP) {
        destination = std::format("D#{}", row.dest_shipno.value_or(0));
      } else {
        destination = std::format("L{}", row.land_coords);
      }
    } else if (row.navigating) {
      destination = std::format("nav:{} ({})", row.nav_bearing, row.nav_turns);
    } else {
      destination = row.dest_path;
    }

    table.add_row(
        {std::format("{}", row.type_letter), std::format("{}", row.number),
         name_str, std::format("{}", row.governor),
         std::format("{}", row.damage), std::format("{}", row.popn),
         std::format("{}", row.troops), std::format("{}", row.destruct),
         std::format("{:.0f}", row.fuel), speed_indicator, row.orbits,
         destination});
  }

  if (has_rows) {
    std::format_to(std::back_inserter(out), "{}\n", table.str());
  }
}

}  // namespace

std::string render_rst(const RstViewModel& vm) {
  std::string out;
  if (vm.flags.ship) {
    for (const auto& entry : vm.ships) {
      const std::span<const RstShipEntry> single{&entry, 1};
      append_stock_table(out, single);
      append_status_table(out, single);
      append_weapons_table(out, single);
      append_factory_table(out, single);
      append_general_table(out, single);
    }
    return out;
  }

  const std::span<const RstShipEntry> all{vm.ships};
  if (vm.flags.stock) append_stock_table(out, all);
  if (vm.flags.status) append_status_table(out, all);
  if (vm.flags.weapons) append_weapons_table(out, all);
  if (vm.flags.factories) append_factory_table(out, all);
  if (vm.flags.report) append_general_table(out, all);
  return out;
}

std::string render_json_rst(const RstViewModel& vm) {
  return render_json_envelope("rst", vm);
}

std::string render_for_mode(UiMode mode, const RstViewModel& vm) {
  return mode == UiMode::JSON ? render_json_rst(vm) : render_rst(vm);
}

}  // namespace GB::presentation
