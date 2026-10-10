// SPDX-License-Identifier: Apache-2.0

/// \file core.cppm
/// \brief Core presentation dispatch (present_to / render_for_mode) and Glaze
/// JSON envelope serialization.

export module gb.presentation.core;

import strong_id;
import glaze.core;
import glaze.json;
import gb.entities;
import gb.services;
import gb.repositories.glaze;
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

/// Concept satisfied by any ViewModel that provides a non-member
/// `render_for_mode(UiMode, const T&)` overload in `namespace
/// GB::presentation`.
template <typename T>
concept HasModeRenderOverload = requires(UiMode mode, const T& vm) {
  { render_for_mode(mode, vm) } -> std::same_as<std::string>;
};

/// Polymorphically formats `vm` according to `mode` and writes it to `out`.
/// Found via ADL from `GameObj::present(const ViewModel&)` because `UiMode`
/// resides in `namespace GB::presentation`.
template <HasModeRenderOverload ViewModel>
void present_to(std::ostream& out, UiMode mode, const ViewModel& vm) {
  out << render_for_mode(mode, vm);
}

}  // namespace GB::presentation
