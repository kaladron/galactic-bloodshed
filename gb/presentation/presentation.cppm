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

/// Dispatches rendering of `data` according to `mode`:
/// - `UiMode::ASCII`: invokes `ascii_renderer(data)`
/// - `UiMode::JSON`: serializes `JsonEnvelope<T>{type, &data}` via Glaze
template <typename T, typename AsciiRenderer>
  requires std::invocable<AsciiRenderer, const T&>
[[nodiscard]] std::string render_by_mode(UiMode mode, std::string_view type,
                                         const T& data,
                                         AsciiRenderer&& ascii_renderer) {
  std::string out;
  if (mode == UiMode::JSON) {
    out = render_json_envelope(type, data);
  } else {
    out = std::invoke(std::forward<AsciiRenderer>(ascii_renderer), data);
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
