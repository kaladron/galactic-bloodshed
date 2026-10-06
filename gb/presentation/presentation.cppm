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
/// Decomposes `TripEstimate` via structured binding so adding any field to
/// `TripEstimate` triggers a compile-time error until handled here.
[[nodiscard]] inline std::string render_trip_estimate(const TripEstimate& est) {
  const auto& [distance, segments, fuel_used, launch_gravity_fuel,
               launch_planet_name, arrival_status, estimated_arrival_time] =
      est;

  std::string out;
  if (launch_gravity_fuel > 0.00) {
    std::format_to(
        std::back_inserter(out),
        "Total Distance = {:.2f}   Number of Segments = {}\nFuel = {:.2f} "
        "({:.2f} used to launch from {})\n  ",
        distance, segments, fuel_used, launch_gravity_fuel, launch_planet_name);
  } else {
    std::format_to(
        std::back_inserter(out),
        "Total Distance = {:.2f}   Number of Segments = {}\nFuel = {:.2f}   ",
        distance, segments, fuel_used);
  }

  switch (arrival_status) {
    case ArrivalTimeStatus::ServerStateUnavailable:
      out += "Server state unavailable.\n";
      break;
    case ArrivalTimeStatus::SegmentDiscrepancy:
      out += "Estimated arrival time not available due to segment # "
             "discrepancy.\n";
      break;
    case ArrivalTimeStatus::Available: {
      std::time_t arrival = estimated_arrival_time;
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
/// Decomposes `PlanetMapViewModel`, `PlanetMapCell`, and `PlanetAlienPresence`
/// via structured bindings so adding any field triggers a compile-time error
/// until handled here.
[[nodiscard]] inline std::string
render_ascii_planet_map(const PlanetMapViewModel& vm) {
  const auto& [planet_name, dimensions, sectors, planet_type_name, is_metamorph,
               sectors_owned, aliens_unknown, aliens, guns, mob_points, comread,
               mob_set, compatibility, toxicity, resource_stockpile,
               fuel_stockpile, destruct_cap, player_popn, total_popn,
               effective_maxpopn, crystals, player_troops, total_troops,
               total_resources, tax, newtax, est_production, slaved_to,
               primary_unstable] = vm;

  std::string out;
  std::format_to(std::back_inserter(out), "     {}\n", planet_name);

  if (dimensions.x >= 10) {
    out += "   ";
    for (const int x : std::views::iota(0, dimensions.x)) {
      out.push_back(static_cast<char>('0' + ((x / 10) % 10)));
    }
    out.push_back('\n');
  }

  out += "   ";
  for (const int x : std::views::iota(0, dimensions.x)) {
    out.push_back(static_cast<char>('0' + (x % 10)));
  }
  out.push_back('\n');

  if (dimensions.x > 0 && dimensions.y > 0) {
    bool in_inverse = false;
    for (const auto& [coords, _, glyph, inverse] : sectors) {
      if (coords.x == 0) {
        std::format_to(std::back_inserter(out), "{:02d} ", coords.y);
      }
      if (inverse && !in_inverse) {
        out += "\x1b[7m";
        in_inverse = true;
      } else if (!inverse && in_inverse) {
        out += "\x1b[27m";
        in_inverse = false;
      }
      out.push_back(glyph);
      if (coords.x + 1 == dimensions.x) {
        if (in_inverse) {
          out += "\x1b[27m";
          in_inverse = false;
        }
        out.push_back('\n');
      }
    }
  }
  out.push_back('\n');

  std::format_to(std::back_inserter(out),
                 "Type: {:<8}   Sects {:<7}: {:<3}   Aliens:", planet_type_name,
                 is_metamorph ? "covered" : "owned", sectors_owned);
  if (aliens_unknown) {
    out += "???";
  } else if (aliens.empty()) {
    out += "(none)";
  } else {
    for (const auto& [alien_player, at_war] : aliens) {
      std::format_to(std::back_inserter(out), "{}{}", at_war ? '*' : ' ',
                     alien_player);
    }
  }
  out.push_back('\n');

  std::string compat_str = std::format("{:.2f}%", compatibility);
  if (toxicity > 50) {
    std::format_to(std::back_inserter(compat_str), " ({}% TOXIC)", toxicity);
  }

  tabulate::Table stats_table;
  stats_table.format().hide_border().column_separator(" ");
  stats_table.column(0).format().width(20).font_align(
      tabulate::FontAlign::right);
  stats_table.column(1).format().width(10);
  stats_table.column(2).format().width(20).font_align(
      tabulate::FontAlign::right);
  stats_table.column(3).format().width(26);

  stats_table.add_row({"Guns :", std::format("{}", guns),
                       "Mob Points :", std::format("{}", mob_points)});
  stats_table.add_row(
      {"Mobilization :", std::format("{} ({})", comread, mob_set),
       "Compatibility :", compat_str});
  stats_table.add_row(
      {"Resource stockpile :", std::format("{}", resource_stockpile),
       "Fuel stockpile :", std::format("{}", fuel_stockpile)});
  stats_table.add_row(
      {"Destruct cap :", std::format("{}", destruct_cap),
       is_metamorph ? "Tons of biomass :" : "Total Population :",
       std::format("{} ({}/{})", player_popn, total_popn, effective_maxpopn)});
  stats_table.add_row(
      {"Crystals :", std::format("{}", crystals),
       "Ground forces :", std::format("{} ({})", player_troops, total_troops)});

  out += stats_table.str();
  out.push_back('\n');

  std::format_to(std::back_inserter(out),
                 "{} Total Resource Deposits     Tax rate {}%  New {}%\n"
                 "Estimated Production Next Update : {:.2f}\n",
                 total_resources, tax, newtax, est_production);

  if (slaved_to) {
    std::format_to(std::back_inserter(out), "      ENSLAVED to player {};\n",
                   *slaved_to);
  }
  if (primary_unstable) {
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
/// Decomposes `PlanetBuildError` via structured binding.
[[nodiscard]] inline std::string
format_planet_build_error(const PlanetBuildError& err) {
  const auto& [reason, enslaving_player] = err;
  std::string out;
  switch (reason) {
    case PlanetBuildErrorReason::EnslavedByForeignPlayer:
      std::format_to(std::back_inserter(out),
                     "This planet is enslaved by player {}.\n",
                     enslaving_player);
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
/// Decomposes `CreatedShipSummary` via structured binding.
[[nodiscard]] inline std::string
render_created_ship_summary(const CreatedShipSummary& summary) {
  const auto& [ship_display, build_cost, tech, landed_sector, previous_toxicity,
               updated_toxicity] = summary;

  std::string out;
  if (previous_toxicity && updated_toxicity) {
    std::format_to(std::back_inserter(out),
                   "Toxin concentration on planet was {}%, now {}%.\n",
                   *previous_toxicity, *updated_toxicity);
  }
  std::format_to(std::back_inserter(out),
                 "{} built at a cost of {} resources.\nTechnology {:.1f}.\n",
                 ship_display, build_cost, tech);
  if (landed_sector) {
    std::format_to(std::back_inserter(out), "{} is on sector {}.\n",
                   ship_display, *landed_sector);
  }
  return out;
}

/// Formats an `InitializedShipReport` into its ASCII presentation string.
/// Decomposes `InitializedShipReport` via structured binding.
[[nodiscard]] inline std::string
render_initialized_ship_report(const InitializedShipReport& report) {
  const auto& [ship_type, tele_range, damage, can_repair, has_crew_capacity,
               loaded_crew, loaded_fuel] = report;

  std::string out;
  switch (ship_type) {
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
                     tele_range);
      break;
    default:
      break;
  }
  if (damage) {
    std::format_to(
        std::back_inserter(out),
        "Warning: This ship is constructed with a {}% damage level.\n", damage);
    if (!can_repair && has_crew_capacity) {
      out += "It will need resources to become fully operational.\n";
    }
  }
  if (can_repair && has_crew_capacity) {
    out += "This ship does not need resources to repair.\n";
  }
  if (ship_type == ShipType::OTYPE_FACTORY) {
    out += "This factory may not begin repairs until it has been activated.\n";
  }
  if (!has_crew_capacity) {
    out += "This ship is robotic, and may not repair itself.\n";
  }

  std::format_to(std::back_inserter(out),
                 "Loaded with {} crew and {:.1f} fuel.\n", loaded_crew,
                 loaded_fuel);
  return out;
}

/// Formats a `CapturedShipsReport` into its ASCII presentation string.
/// Decomposes `CapturedShipsReport` and `CapturedShipEvent` via structured
/// bindings.
[[nodiscard]] inline std::string
render_captured_ships_report(const CapturedShipsReport& report) {
  const auto& [captured_ships] = report;
  std::string out;
  for (const auto& [_, ship_display, _, _] : captured_ships) {
    std::format_to(std::back_inserter(out), "{} CAPTURED!\n", ship_display);
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

export namespace glz {

template <typename T>
struct meta<GB::presentation::JsonEnvelope<T>> {
  using V = GB::presentation::JsonEnvelope<T>;
  static constexpr auto value = object("type", &V::type, "data", &V::data);
};

template <>
struct meta<ArrivalTimeStatus> {
  using enum ArrivalTimeStatus;
  static constexpr auto value = enumerate(
      "available", Available, "server_state_unavailable",
      ServerStateUnavailable, "segment_discrepancy", SegmentDiscrepancy);
};

}  // namespace glz
