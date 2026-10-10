// SPDX-License-Identifier: Apache-2.0

/// \file construction.cc
/// \brief Implementation of ship construction presentation formatters.

module gb.presentation.construction;

import gb.presentation.core;
import glaze.core;
import glaze.json;
import gb.repositories.glaze;
import strong_id;

namespace GB::presentation {

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

}  // namespace GB::presentation
