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
    for (const auto& [coords, owner, glyph, inverse] : sectors) {
      (void)owner;
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

/// Abstract base class for polymorphic UI presentation across wire protocols.
class Presenter {
public:
  virtual ~Presenter() = default;

  [[nodiscard]] virtual std::string render(const TripEstimate& vm) const = 0;
  [[nodiscard]] virtual std::string
  render(const PlanetMapViewModel& vm) const = 0;
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
