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

}  // namespace GB::presentation

export namespace glz {

template <typename T>
struct meta<GB::presentation::JsonEnvelope<T>> {
  using V = GB::presentation::JsonEnvelope<T>;
  static constexpr auto value = object("type", &V::type, "data", &V::data);
};

}  // namespace glz
